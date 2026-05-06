#include "handlers.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>

#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/redis/component.hpp>

#include "http_utils.hpp"
#include "security.hpp"

namespace hotel_booking_pg {
namespace {


const userver::storages::redis::CommandControl kRedisCc{
    std::chrono::seconds{5},
    std::chrono::seconds{30},
    2
};

constexpr std::chrono::seconds kHotelsListTtl{60};
constexpr std::chrono::seconds kHotelsCityTtl{120};
constexpr std::chrono::seconds kUserBookingsTtl{30};
constexpr int kLoginLimit = 5;
constexpr int kCreateBookingLimit = 10;
constexpr int kCreateHotelLimit = 20;
constexpr int kRateWindowSeconds = 60;

std::int64_t UnixNow() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string NormalizeKeyPart(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        if (std::isalnum(ch) != 0) return static_cast<char>(std::tolower(ch));
        return '_';
    });
    return value;
}

void SetCacheHeaders(userver::server::http::HttpRequest& request, std::string_view state, std::chrono::seconds ttl) {
    auto& response = request.GetHttpResponse();
    response.SetHeader(std::string_view{"X-Cache"}, std::string(state));
    response.SetHeader(std::string_view{"X-Cache-TTL"}, std::to_string(ttl.count()));
}

std::optional<std::string> ReadCache(userver::server::http::HttpRequest& request,
                                    const userver::storages::redis::ClientPtr& redis,
                                    const std::string& key,
                                    std::chrono::seconds ttl) {
    const auto cached = redis->Get(key, kRedisCc).Get();
    if (cached) {
        SetCacheHeaders(request, "HIT", ttl);
        return *cached;
    }
    SetCacheHeaders(request, "MISS", ttl);
    return std::nullopt;
}

void SaveCache(const userver::storages::redis::ClientPtr& redis,
               const std::string& key,
               const std::string& value,
               std::chrono::seconds ttl) {
    redis->Setex(key, ttl, value, kRedisCc).Get();
}

void DeleteCache(const userver::storages::redis::ClientPtr& redis, const std::string& key) {
    redis->Del(key, kRedisCc).Get();
}

struct RateLimitResult {
    bool allowed{true};
    int limit{0};
    int remaining{0};
    std::int64_t reset_at{0};
};

RateLimitResult CheckFixedWindow(userver::server::http::HttpRequest& request,
                                 const userver::storages::redis::ClientPtr& redis,
                                 const std::string& key,
                                 int limit,
                                 int window_seconds) {
    const auto current = static_cast<std::int64_t>(redis->Incr(key, kRedisCc).Get());
    if (current == 1) {
        redis->Expire(key, std::chrono::seconds{window_seconds}, kRedisCc).Get();
    }

    RateLimitResult result;
    result.limit = limit;
    result.remaining = static_cast<int>(std::max<std::int64_t>(0, static_cast<std::int64_t>(limit) - current));
    result.reset_at = UnixNow() + window_seconds;
    result.allowed = current <= limit;

    auto& response = request.GetHttpResponse();
    response.SetHeader(std::string_view{"X-RateLimit-Limit"}, std::to_string(result.limit));
    response.SetHeader(std::string_view{"X-RateLimit-Remaining"}, std::to_string(result.remaining));
    response.SetHeader(std::string_view{"X-RateLimit-Reset"}, std::to_string(result.reset_at));
    if (!result.allowed) {
        response.SetHeader(std::string_view{"Retry-After"}, std::to_string(window_seconds));
    }
    return result;
}

std::string RateLimitExceeded(userver::server::http::HttpRequest& request) {
    return Fail(request, userver::server::http::HttpStatus::kTooManyRequests, "Rate limit exceeded");
}

std::string Trim(std::string value) {
    const auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
    value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), is_space));
    value.erase(std::find_if_not(value.rbegin(), value.rend(), is_space).base(), value.end());
    return value;
}

std::optional<SessionData> Authorize(const userver::server::http::HttpRequest& request, const TokenStore& tokens) {
    const auto token = GetBearerToken(request);
    if (token.empty()) return std::nullopt;
    return tokens.Resolve(token);
}

std::optional<std::int64_t> ParseInt64(std::string_view value) {
    try {
        std::size_t pos = 0;
        const auto result = std::stoll(std::string(value), &pos);
        if (pos != value.size()) return std::nullopt;
        return result;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

userver::formats::json::ValueBuilder UserJson(const userver::storages::postgres::Row& row) {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = row["id"].As<std::int64_t>();
    builder["login"] = row["login"].As<std::string>();
    builder["first_name"] = row["first_name"].As<std::string>();
    builder["last_name"] = row["last_name"].As<std::string>();
    builder["created_at"] = row["created_at"].As<std::string>();
    return builder;
}

userver::formats::json::ValueBuilder HotelJson(const userver::storages::postgres::Row& row) {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = row["id"].As<std::int64_t>();
    builder["name"] = row["name"].As<std::string>();
    builder["city"] = row["city"].As<std::string>();
    builder["address"] = row["address"].As<std::string>();
    builder["stars"] = row["stars"].As<int>();
    builder["rooms_total"] = row["rooms_total"].As<int>();
    builder["created_by"] = row["created_by"].As<std::int64_t>();
    builder["created_at"] = row["created_at"].As<std::string>();
    return builder;
}

userver::formats::json::ValueBuilder BookingJson(const userver::storages::postgres::Row& row) {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = row["id"].As<std::int64_t>();
    builder["user_id"] = row["user_id"].As<std::int64_t>();
    builder["hotel_id"] = row["hotel_id"].As<std::int64_t>();
    builder["check_in"] = row["check_in"].As<std::string>();
    builder["check_out"] = row["check_out"].As<std::string>();
    builder["guests"] = row["guests"].As<int>();
    builder["status"] = row["status"].As<std::string>();
    builder["created_at"] = row["created_at"].As<std::string>();
    if (row["cancelled_at"].IsNull()) {
        builder["cancelled_at"] = nullptr;
    } else {
        builder["cancelled_at"] = row["cancelled_at"].As<std::string>();
    }
    return builder;
}

std::string SingleObject(userver::formats::json::ValueBuilder builder) {
    return userver::formats::json::ToString(builder.ExtractValue());
}

std::string SelectUserByIdQuery() {
    return R"(
        SELECT
            id,
            login,
            first_name,
            last_name,
            to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS"Z"') AS created_at
        FROM users
        WHERE id = $1
    )";
}

std::string SelectHotelByIdQuery() {
    return R"(
        SELECT
            id,
            name,
            city,
            address,
            stars,
            rooms_total,
            created_by,
            to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS"Z"') AS created_at
        FROM hotels
        WHERE id = $1
    )";
}

std::string SelectBookingByIdQuery() {
    return R"(
        SELECT
            id,
            user_id,
            hotel_id,
            to_char(check_in, 'YYYY-MM-DD') AS check_in,
            to_char(check_out, 'YYYY-MM-DD') AS check_out,
            guests,
            status::text AS status,
            to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS"Z"') AS created_at,
            CASE
                WHEN cancelled_at IS NULL THEN NULL
                ELSE to_char(cancelled_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS"Z"')
            END AS cancelled_at
        FROM bookings
        WHERE id = $1
    )";
}

}  // namespace

std::string RootHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                       userver::server::request::RequestContext&) const {
    PrepareJson(request);
    userver::formats::json::ValueBuilder builder;
    builder["message"] = "Hotel Booking userver API with PostgreSQL, Redis cache and rate limiting is running";
    return userver::formats::json::ToString(builder.ExtractValue());
}

RegisterUserHandler::RegisterUserHandler(const userver::components::ComponentConfig& config,
                                         const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()) {}

std::string RegisterUserHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                               userver::server::request::RequestContext&) const {
    PrepareJson(request);
    std::string parse_error;
    const auto body = ParseBody(request, parse_error);
    if (!body) return ErrorBody(parse_error);

    try {
        const auto login = Trim((*body)["login"].As<std::string>());
        const auto password = (*body)["password"].As<std::string>();
        const auto first_name = Trim((*body)["first_name"].As<std::string>());
        const auto last_name = Trim((*body)["last_name"].As<std::string>());

        if (login.size() < 3 || password.size() < 6 || first_name.empty() || last_name.empty()) {
            return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid user payload");
        }

        const auto existing = pg_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                                           "SELECT id FROM users WHERE login = $1",
                                           login);
        if (!existing.IsEmpty()) {
            return Fail(request, userver::server::http::HttpStatus::kConflict, "User with this login already exists");
        }

        const auto created = pg_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                                          R"(
                                              INSERT INTO users (login, password_hash, first_name, last_name)
                                              VALUES ($1, $2, $3, $4)
                                              RETURNING id
                                          )",
                                          login,
                                          HashPassword(password),
                                          first_name,
                                          last_name);
        const auto user_id = created.AsSingleRow<std::int64_t>();
        const auto selected = pg_->Execute(userver::storages::postgres::ClusterHostType::kSlave, SelectUserByIdQuery(), user_id);

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return SingleObject(UserJson(selected.Front()));
    } catch (const std::exception&) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid user payload");
    }
}

LoginHandler::LoginHandler(const userver::components::ComponentConfig& config,
                           const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()),
      redis_(context.FindComponent<userver::components::Redis>("redis-cache").GetClient("cache")),
      tokens_(context.FindComponent<TokenStore>()) {}

std::string LoginHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                        userver::server::request::RequestContext&) const {
    PrepareJson(request);
    std::string parse_error;
    const auto body = ParseBody(request, parse_error);
    if (!body) return ErrorBody(parse_error);

    try {
        const auto login = Trim((*body)["login"].As<std::string>());
        const auto password = (*body)["password"].As<std::string>();

        const auto rate = CheckFixedWindow(
            request,
            redis_,
            "rate:login:" + NormalizeKeyPart(login),
            kLoginLimit,
            kRateWindowSeconds
        );
        if (!rate.allowed) {
            return RateLimitExceeded(request);
        }

        const auto result = pg_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                                         R"(
                                             SELECT id, login, password_hash
                                             FROM users
                                             WHERE login = $1
                                         )",
                                         login);
        if (result.IsEmpty()) {
            return Fail(request, userver::server::http::HttpStatus::kUnauthorized, "Invalid login or password");
        }

        const auto row = result.Front();
        if (!VerifyPassword(password, row["password_hash"].As<std::string>())) {
            return Fail(request, userver::server::http::HttpStatus::kUnauthorized, "Invalid login or password");
        }

        userver::formats::json::ValueBuilder builder;
        builder["access_token"] = tokens_.Issue(std::to_string(row["id"].As<std::int64_t>()), row["login"].As<std::string>());
        builder["token_type"] = "bearer";
        return userver::formats::json::ToString(builder.ExtractValue());
    } catch (const std::exception&) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid login payload");
    }
}

GetUserByLoginHandler::GetUserByLoginHandler(const userver::components::ComponentConfig& config,
                                             const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()) {}

std::string GetUserByLoginHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                                 userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const auto login = request.GetPathArg("login");
    if (login.size() < 3) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid login");
    }

    const auto result = pg_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                                     R"(
                                         SELECT
                                             id,
                                             login,
                                             first_name,
                                             last_name,
                                             to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS"Z"') AS created_at
                                         FROM users
                                         WHERE login = $1
                                     )",
                                     login);
    if (result.IsEmpty()) {
        return Fail(request, userver::server::http::HttpStatus::kNotFound, "User not found");
    }
    return SingleObject(UserJson(result.Front()));
}

SearchUsersHandler::SearchUsersHandler(const userver::components::ComponentConfig& config,
                                       const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()) {}

std::string SearchUsersHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                              userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const auto mask = Trim(request.GetArg("mask"));
    if (mask.empty()) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "mask is required");
    }

    const auto result = pg_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                                     R"(
                                         SELECT
                                             id,
                                             login,
                                             first_name,
                                             last_name,
                                             to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS"Z"') AS created_at
                                         FROM users
                                         WHERE lower(first_name || ' ' || last_name) LIKE '%' || lower($1) || '%'
                                         ORDER BY id
                                     )",
                                     mask);

    userver::formats::json::ValueBuilder array(userver::formats::common::Type::kArray);
    for (const auto& row : result) {
        array.PushBack(UserJson(row));
    }
    return userver::formats::json::ToString(array.ExtractValue());
}

CreateHotelHandler::CreateHotelHandler(const userver::components::ComponentConfig& config,
                                       const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()),
      redis_(context.FindComponent<userver::components::Redis>("redis-cache").GetClient("cache")),
      tokens_(context.FindComponent<TokenStore>()) {}

std::string CreateHotelHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                              userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const auto session = Authorize(request, tokens_);
    if (!session) {
        return Fail(request, userver::server::http::HttpStatus::kUnauthorized, "Authentication required");
    }

    const auto rate = CheckFixedWindow(
        request,
        redis_,
        "rate:hotel-create:user:" + session->user_id,
        kCreateHotelLimit,
        kRateWindowSeconds
    );
    if (!rate.allowed) {
        return RateLimitExceeded(request);
    }

    std::string parse_error;
    const auto body = ParseBody(request, parse_error);
    if (!body) return ErrorBody(parse_error);

    try {
        const auto name = Trim((*body)["name"].As<std::string>());
        const auto city = Trim((*body)["city"].As<std::string>());
        const auto address = Trim((*body)["address"].As<std::string>());
        const auto stars = (*body)["stars"].As<int>();
        const auto rooms_total = (*body)["rooms_total"].As<int>();

        if (name.size() < 2 || city.size() < 2 || address.size() < 5 || stars < 1 || stars > 5 || rooms_total < 1) {
            return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid hotel payload");
        }

        const auto created = pg_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                                          R"(
                                              INSERT INTO hotels (name, city, address, stars, rooms_total, created_by)
                                              VALUES ($1, $2, $3, $4, $5, $6)
                                              RETURNING id
                                          )",
                                          name,
                                          city,
                                          address,
                                          stars,
                                          rooms_total,
                                          std::stoll(session->user_id));
        const auto hotel_id = created.AsSingleRow<std::int64_t>();
        const auto selected = pg_->Execute(userver::storages::postgres::ClusterHostType::kSlave, SelectHotelByIdQuery(), hotel_id);

        DeleteCache(redis_, "cache:hotels:list:v1");
        DeleteCache(redis_, "cache:hotels:city:" + NormalizeKeyPart(city) + ":v1");

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return SingleObject(HotelJson(selected.Front()));
    } catch (const std::exception&) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid hotel payload");
    }
}

ListHotelsHandler::ListHotelsHandler(const userver::components::ComponentConfig& config,
                                     const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()),
      redis_(context.FindComponent<userver::components::Redis>("redis-cache").GetClient("cache")) {}

std::string ListHotelsHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                             userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const std::string cache_key = "cache:hotels:list:v1";
    if (const auto cached = ReadCache(request, redis_, cache_key, kHotelsListTtl)) {
        return *cached;
    }

    const auto result = pg_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                                     R"(
                                         SELECT
                                             id,
                                             name,
                                             city,
                                             address,
                                             stars,
                                             rooms_total,
                                             created_by,
                                             to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS"Z"') AS created_at
                                         FROM hotels
                                         ORDER BY id
                                     )");
    userver::formats::json::ValueBuilder array(userver::formats::common::Type::kArray);
    for (const auto& row : result) {
        array.PushBack(HotelJson(row));
    }
    const auto body = userver::formats::json::ToString(array.ExtractValue());
    SaveCache(redis_, cache_key, body, kHotelsListTtl);
    return body;
}

SearchHotelsHandler::SearchHotelsHandler(const userver::components::ComponentConfig& config,
                                         const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()),
      redis_(context.FindComponent<userver::components::Redis>("redis-cache").GetClient("cache")) {}

std::string SearchHotelsHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                               userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const auto city = Trim(request.GetArg("city"));
    if (city.empty()) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "city is required");
    }

    const std::string cache_key = "cache:hotels:city:" + NormalizeKeyPart(city) + ":v1";
    if (const auto cached = ReadCache(request, redis_, cache_key, kHotelsCityTtl)) {
        return *cached;
    }

    const auto result = pg_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                                     R"(
                                         SELECT
                                             id,
                                             name,
                                             city,
                                             address,
                                             stars,
                                             rooms_total,
                                             created_by,
                                             to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS"Z"') AS created_at
                                         FROM hotels
                                         WHERE lower(city) LIKE '%' || lower($1) || '%'
                                         ORDER BY id
                                     )",
                                     city);
    userver::formats::json::ValueBuilder array(userver::formats::common::Type::kArray);
    for (const auto& row : result) {
        array.PushBack(HotelJson(row));
    }
    const auto body = userver::formats::json::ToString(array.ExtractValue());
    SaveCache(redis_, cache_key, body, kHotelsCityTtl);
    return body;
}

CreateBookingHandler::CreateBookingHandler(const userver::components::ComponentConfig& config,
                                           const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()),
      redis_(context.FindComponent<userver::components::Redis>("redis-cache").GetClient("cache")),
      tokens_(context.FindComponent<TokenStore>()) {}

std::string CreateBookingHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                                userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const auto session = Authorize(request, tokens_);
    if (!session) {
        return Fail(request, userver::server::http::HttpStatus::kUnauthorized, "Authentication required");
    }

    const auto rate = CheckFixedWindow(
        request,
        redis_,
        "rate:booking-create:user:" + session->user_id,
        kCreateBookingLimit,
        kRateWindowSeconds
    );
    if (!rate.allowed) {
        return RateLimitExceeded(request);
    }

    std::string parse_error;
    const auto body = ParseBody(request, parse_error);
    if (!body) return ErrorBody(parse_error);

    try {
        const auto hotel_id = (*body)["hotel_id"].As<std::int64_t>();
        const auto check_in = (*body)["check_in"].As<std::string>();
        const auto check_out = (*body)["check_out"].As<std::string>();
        const auto guests = (*body)["guests"].As<int>();

        if (!IsIsoDate(check_in) || !IsIsoDate(check_out) || check_out <= check_in || guests < 1 || guests > 10) {
            return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid booking payload");
        }

        const auto hotel_result = pg_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                                               "SELECT id, rooms_total FROM hotels WHERE id = $1",
                                               hotel_id);
        if (hotel_result.IsEmpty()) {
            return Fail(request, userver::server::http::HttpStatus::kNotFound, "Hotel not found");
        }

        const auto hotel_row = hotel_result.Front();
        const auto active_count_result = pg_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                                                      R"(
                                                          SELECT COUNT(*)
                                                          FROM bookings
                                                          WHERE hotel_id = $1
                                                            AND status = 'active'
                                                            AND stay_period && daterange($2::date, $3::date, '[)')
                                                      )",
                                                      hotel_id,
                                                      check_in,
                                                      check_out);
        const auto active_count = active_count_result.AsSingleRow<std::int64_t>();
        if (active_count >= hotel_row["rooms_total"].As<int>()) {
            return Fail(request, userver::server::http::HttpStatus::kConflict, "No available rooms for selected dates");
        }

        const auto created = pg_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                                          R"(
                                              INSERT INTO bookings (user_id, hotel_id, check_in, check_out, guests)
                                              VALUES ($1, $2, $3::date, $4::date, $5)
                                              RETURNING id
                                          )",
                                          std::stoll(session->user_id),
                                          hotel_id,
                                          check_in,
                                          check_out,
                                          guests);
        const auto booking_id = created.AsSingleRow<std::int64_t>();
        const auto selected = pg_->Execute(userver::storages::postgres::ClusterHostType::kSlave, SelectBookingByIdQuery(), booking_id);

        DeleteCache(redis_, "cache:users:" + session->user_id + ":bookings:v1");

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return SingleObject(BookingJson(selected.Front()));
    } catch (const std::exception&) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid booking payload");
    }
}

ListUserBookingsHandler::ListUserBookingsHandler(const userver::components::ComponentConfig& config,
                                                 const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()),
      redis_(context.FindComponent<userver::components::Redis>("redis-cache").GetClient("cache")),
      tokens_(context.FindComponent<TokenStore>()) {}

std::string ListUserBookingsHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                                   userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const auto session = Authorize(request, tokens_);
    if (!session) {
        return Fail(request, userver::server::http::HttpStatus::kUnauthorized, "Authentication required");
    }

    const auto user_id_raw = request.GetPathArg("user_id");
    const auto user_id = ParseInt64(user_id_raw);
    if (!user_id) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid user_id");
    }
    if (session->user_id != std::to_string(*user_id)) {
        return Fail(request, userver::server::http::HttpStatus::kForbidden, "Access denied: you can view only your own bookings");
    }

    const std::string cache_key = "cache:users:" + std::to_string(*user_id) + ":bookings:v1";
    if (const auto cached = ReadCache(request, redis_, cache_key, kUserBookingsTtl)) {
        return *cached;
    }

    const auto result = pg_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                                     R"(
                                         SELECT
                                             id,
                                             user_id,
                                             hotel_id,
                                             to_char(check_in, 'YYYY-MM-DD') AS check_in,
                                             to_char(check_out, 'YYYY-MM-DD') AS check_out,
                                             guests,
                                             status::text AS status,
                                             to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS"Z"') AS created_at,
                                             CASE
                                                 WHEN cancelled_at IS NULL THEN NULL
                                                 ELSE to_char(cancelled_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS"Z"')
                                             END AS cancelled_at
                                         FROM bookings
                                         WHERE user_id = $1
                                         ORDER BY created_at DESC
                                     )",
                                     *user_id);

    userver::formats::json::ValueBuilder array(userver::formats::common::Type::kArray);
    for (const auto& row : result) {
        array.PushBack(BookingJson(row));
    }
    const auto body = userver::formats::json::ToString(array.ExtractValue());
    SaveCache(redis_, cache_key, body, kUserBookingsTtl);
    return body;
}

CancelBookingHandler::CancelBookingHandler(const userver::components::ComponentConfig& config,
                                           const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()),
      redis_(context.FindComponent<userver::components::Redis>("redis-cache").GetClient("cache")),
      tokens_(context.FindComponent<TokenStore>()) {}

std::string CancelBookingHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                                userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const auto session = Authorize(request, tokens_);
    if (!session) {
        return Fail(request, userver::server::http::HttpStatus::kUnauthorized, "Authentication required");
    }

    const auto booking_id_raw = request.GetPathArg("booking_id");
    const auto booking_id = ParseInt64(booking_id_raw);
    if (!booking_id) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid booking_id");
    }

    const auto current = pg_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                                      "SELECT user_id, status::text AS status FROM bookings WHERE id = $1",
                                      *booking_id);
    if (current.IsEmpty()) {
        return Fail(request, userver::server::http::HttpStatus::kNotFound, "Booking not found");
    }

    const auto current_row = current.Front();
    if (session->user_id != std::to_string(current_row["user_id"].As<std::int64_t>())) {
        return Fail(request, userver::server::http::HttpStatus::kForbidden, "Access denied: you can cancel only your own booking");
    }
    if (current_row["status"].As<std::string>() == "cancelled") {
        return Fail(request, userver::server::http::HttpStatus::kConflict, "Booking already cancelled");
    }

    pg_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                 R"(
                     UPDATE bookings
                     SET status = 'cancelled',
                         cancelled_at = NOW()
                     WHERE id = $1
                 )",
                 *booking_id);
    const auto selected = pg_->Execute(userver::storages::postgres::ClusterHostType::kSlave, SelectBookingByIdQuery(), *booking_id);
    DeleteCache(redis_, "cache:users:" + session->user_id + ":bookings:v1");
    return SingleObject(BookingJson(selected.Front()));
}

}  // namespace hotel_booking_pg
