#include "handlers.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <string>
#include <vector>

#include <userver/formats/bson.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/utils/datetime.hpp>

#include "http_utils.hpp"
#include "security.hpp"

namespace hotel_booking_mongo {
namespace {

using userver::formats::bson::MakeArray;
using userver::formats::bson::MakeDoc;
using userver::formats::bson::Oid;

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

std::chrono::system_clock::time_point Now() {
    return std::chrono::system_clock::now();
}

std::optional<Oid> ParseOid(std::string_view value) {
    try {
        return Oid{value};
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<std::chrono::system_clock::time_point> ParseDate(std::string_view value) {
    if (!IsIsoDate(value)) return std::nullopt;
    try {
        return userver::utils::datetime::Stringtime(std::string(value) + "T00:00:00Z");
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::string ToIsoTimestamp(std::chrono::system_clock::time_point value) {
    return userver::utils::datetime::Timestring(value);
}

std::string ToIsoDate(std::chrono::system_clock::time_point value) {
    auto text = userver::utils::datetime::Timestring(value);
    if (text.size() >= 10) return text.substr(0, 10);
    return text;
}

userver::formats::json::ValueBuilder UserJson(const userver::formats::bson::Document& doc) {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = doc["_id"].As<Oid>().ToString();
    builder["login"] = doc["login"].As<std::string>();
    builder["first_name"] = doc["first_name"].As<std::string>();
    builder["last_name"] = doc["last_name"].As<std::string>();
    builder["created_at"] = ToIsoTimestamp(doc["created_at"].As<std::chrono::system_clock::time_point>());
    return builder;
}

userver::formats::json::ValueBuilder HotelJson(const userver::formats::bson::Document& doc) {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = doc["_id"].As<Oid>().ToString();
    builder["name"] = doc["name"].As<std::string>();
    builder["city"] = doc["city"].As<std::string>();
    builder["address"] = doc["address"].As<std::string>();
    builder["stars"] = doc["stars"].As<int>();
    builder["rooms_total"] = doc["rooms_total"].As<int>();
    builder["created_by"] = doc["created_by"].As<Oid>().ToString();
    builder["created_at"] = ToIsoTimestamp(doc["created_at"].As<std::chrono::system_clock::time_point>());
    userver::formats::json::ValueBuilder amenities(userver::formats::common::Type::kArray);
    for (const auto& item : doc["amenities"]) {
        amenities.PushBack(item.As<std::string>());
    }
    builder["amenities"] = amenities.ExtractValue();
    return builder;
}

userver::formats::json::ValueBuilder SnapshotJson(const userver::formats::bson::Document& snapshot) {
    userver::formats::json::ValueBuilder builder;
    builder["name"] = snapshot["name"].As<std::string>();
    builder["city"] = snapshot["city"].As<std::string>();
    builder["address"] = snapshot["address"].As<std::string>();
    builder["stars"] = snapshot["stars"].As<int>();
    return builder;
}

userver::formats::json::ValueBuilder BookingJson(const userver::formats::bson::Document& doc) {
    const auto stay = doc["stay"].As<userver::formats::bson::Document>();
    const auto snapshot = doc["hotel_snapshot"].As<userver::formats::bson::Document>();

    userver::formats::json::ValueBuilder builder;
    builder["id"] = doc["_id"].As<Oid>().ToString();
    builder["user_id"] = doc["user_id"].As<Oid>().ToString();
    builder["hotel_id"] = doc["hotel_id"].As<Oid>().ToString();
    builder["check_in"] = ToIsoDate(stay["check_in"].As<std::chrono::system_clock::time_point>());
    builder["check_out"] = ToIsoDate(stay["check_out"].As<std::chrono::system_clock::time_point>());
    builder["guests"] = stay["guests"].As<int>();
    builder["status"] = doc["status"].As<std::string>();
    builder["hotel_snapshot"] = SnapshotJson(snapshot).ExtractValue();
    builder["created_at"] = ToIsoTimestamp(doc["created_at"].As<std::chrono::system_clock::time_point>());
    if (doc["cancelled_at"].IsNull()) {
        builder["cancelled_at"] = nullptr;
    } else {
        builder["cancelled_at"] = ToIsoTimestamp(doc["cancelled_at"].As<std::chrono::system_clock::time_point>());
    }
    return builder;
}

std::string ToJson(userver::formats::json::ValueBuilder builder) {
    return userver::formats::json::ToString(builder.ExtractValue());
}

std::vector<userver::formats::bson::Document> Collect(userver::storages::mongo::Cursor cursor) {
    std::vector<userver::formats::bson::Document> docs;
    if (!cursor) return docs;
    for (const auto& doc : cursor) docs.push_back(doc);
    return docs;
}

}  // namespace

std::string RootHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                       userver::server::request::RequestContext&) const {
    PrepareJson(request);
    userver::formats::json::ValueBuilder builder;
    builder["message"] = "Hotel Booking REST API with MongoDB is running";
    return userver::formats::json::ToString(builder.ExtractValue());
}

RegisterUserHandler::RegisterUserHandler(const userver::components::ComponentConfig& config,
                                         const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pool_(context.FindComponent<userver::components::Mongo>("mongo-db").GetPool()) {}

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

        auto users = pool_->GetCollection("users");
        if (users.FindOne(MakeDoc("login", login))) {
            return Fail(request, userver::server::http::HttpStatus::kConflict, "User with this login already exists");
        }

        const Oid user_id;
        const auto now = Now();
        users.InsertOne(MakeDoc(
            "_id", user_id,
            "login", login,
            "password_hash", HashPassword(password),
            "first_name", first_name,
            "last_name", last_name,
            "created_at", now
        ));
        const auto created = users.FindOne(MakeDoc("_id", user_id));
        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return ToJson(UserJson(*created));
    } catch (const std::exception&) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid user payload");
    }
}

LoginHandler::LoginHandler(const userver::components::ComponentConfig& config,
                           const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pool_(context.FindComponent<userver::components::Mongo>("mongo-db").GetPool()),
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

        auto users = pool_->GetCollection("users");
        const auto user = users.FindOne(MakeDoc("login", login));
        if (!user || !VerifyPassword(password, (*user)["password_hash"].As<std::string>())) {
            return Fail(request, userver::server::http::HttpStatus::kUnauthorized, "Invalid login or password");
        }

        userver::formats::json::ValueBuilder builder;
        builder["access_token"] = tokens_.Issue((*user)["_id"].As<Oid>().ToString(), (*user)["login"].As<std::string>());
        builder["token_type"] = "bearer";
        return userver::formats::json::ToString(builder.ExtractValue());
    } catch (const std::exception&) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid login payload");
    }
}

GetUserByLoginHandler::GetUserByLoginHandler(const userver::components::ComponentConfig& config,
                                             const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pool_(context.FindComponent<userver::components::Mongo>("mongo-db").GetPool()) {}

std::string GetUserByLoginHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                                 userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const auto login = request.GetPathArg("login");
    auto users = pool_->GetCollection("users");
    const auto user = users.FindOne(MakeDoc("login", login));
    if (!user) {
        return Fail(request, userver::server::http::HttpStatus::kNotFound, "User not found");
    }
    return ToJson(UserJson(*user));
}

SearchUsersHandler::SearchUsersHandler(const userver::components::ComponentConfig& config,
                                       const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pool_(context.FindComponent<userver::components::Mongo>("mongo-db").GetPool()) {}

std::string SearchUsersHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                              userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const auto mask = Trim(request.GetArg("mask"));
    if (mask.empty()) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "mask is required");
    }

    auto users = pool_->GetCollection("users");
    auto docs = Collect(users.Find(MakeDoc(
        "$or", MakeArray(
            MakeDoc("first_name", MakeDoc("$regex", mask, "$options", "i")),
            MakeDoc("last_name", MakeDoc("$regex", mask, "$options", "i"))
        )
    )));

    std::sort(docs.begin(), docs.end(), [](const auto& lhs, const auto& rhs) {
        const auto lf = lhs["first_name"].template As<std::string>();
        const auto rf = rhs["first_name"].template As<std::string>();
        if (lf != rf) return lf < rf;
        return lhs["last_name"].template As<std::string>() < rhs["last_name"].template As<std::string>();
    });

    userver::formats::json::ValueBuilder array(userver::formats::common::Type::kArray);
    for (const auto& doc : docs) array.PushBack(UserJson(doc));
    return userver::formats::json::ToString(array.ExtractValue());
}

CreateHotelHandler::CreateHotelHandler(const userver::components::ComponentConfig& config,
                                       const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pool_(context.FindComponent<userver::components::Mongo>("mongo-db").GetPool()),
      tokens_(context.FindComponent<TokenStore>()) {}

std::string CreateHotelHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                              userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const auto session = Authorize(request, tokens_);
    if (!session) {
        return Fail(request, userver::server::http::HttpStatus::kUnauthorized, "Authentication required");
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

        std::vector<std::string> amenities;
        if ((*body).HasMember("amenities") && (*body)["amenities"].IsArray()) {
            for (const auto& item : (*body)["amenities"]) {
                auto amenity = Trim(item.As<std::string>());
                if (!amenity.empty()) amenities.push_back(amenity);
            }
        }
        if (amenities.empty()) amenities.push_back("wifi");
        std::sort(amenities.begin(), amenities.end());
        amenities.erase(std::unique(amenities.begin(), amenities.end()), amenities.end());

        if (name.size() < 2 || city.size() < 2 || address.size() < 5 || stars < 1 || stars > 5 || rooms_total < 1) {
            return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid hotel payload");
        }

        auto hotels = pool_->GetCollection("hotels");
        const Oid hotel_id;
        const auto now = Now();
        userver::formats::bson::Value amenities_array = MakeArray();
        {
            userver::formats::bson::ValueBuilder builder(userver::formats::common::Type::kArray);
            for (const auto& item : amenities) builder.PushBack(item);
            amenities_array = builder.ExtractValue();
        }

        hotels.InsertOne(MakeDoc(
            "_id", hotel_id,
            "name", name,
            "city", city,
            "address", address,
            "stars", stars,
            "rooms_total", rooms_total,
            "amenities", amenities_array,
            "created_by", Oid{session->user_id},
            "created_at", now
        ));

        const auto created = hotels.FindOne(MakeDoc("_id", hotel_id));
        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return ToJson(HotelJson(*created));
    } catch (const std::exception&) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid hotel payload");
    }
}

ListHotelsHandler::ListHotelsHandler(const userver::components::ComponentConfig& config,
                                     const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pool_(context.FindComponent<userver::components::Mongo>("mongo-db").GetPool()) {}

std::string ListHotelsHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                             userver::server::request::RequestContext&) const {
    PrepareJson(request);
    auto hotels = pool_->GetCollection("hotels");
    auto docs = Collect(hotels.Find(MakeDoc()));
    std::sort(docs.begin(), docs.end(), [](const auto& lhs, const auto& rhs) {
        const auto lc = lhs["city"].template As<std::string>();
        const auto rc = rhs["city"].template As<std::string>();
        if (lc != rc) return lc < rc;
        return lhs["name"].template As<std::string>() < rhs["name"].template As<std::string>();
    });

    userver::formats::json::ValueBuilder array(userver::formats::common::Type::kArray);
    for (const auto& doc : docs) array.PushBack(HotelJson(doc));
    return userver::formats::json::ToString(array.ExtractValue());
}

SearchHotelsHandler::SearchHotelsHandler(const userver::components::ComponentConfig& config,
                                         const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pool_(context.FindComponent<userver::components::Mongo>("mongo-db").GetPool()) {}

std::string SearchHotelsHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                               userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const auto city = Trim(request.GetArg("city"));
    if (city.empty()) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "city is required");
    }

    auto hotels = pool_->GetCollection("hotels");
    auto docs = Collect(hotels.Find(MakeDoc("city", MakeDoc("$regex", city, "$options", "i"))));
    std::sort(docs.begin(), docs.end(), [](const auto& lhs, const auto& rhs) {
        return lhs["name"].template As<std::string>() < rhs["name"].template As<std::string>();
    });

    userver::formats::json::ValueBuilder array(userver::formats::common::Type::kArray);
    for (const auto& doc : docs) array.PushBack(HotelJson(doc));
    return userver::formats::json::ToString(array.ExtractValue());
}

CreateBookingHandler::CreateBookingHandler(const userver::components::ComponentConfig& config,
                                           const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pool_(context.FindComponent<userver::components::Mongo>("mongo-db").GetPool()),
      tokens_(context.FindComponent<TokenStore>()) {}

std::string CreateBookingHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                                userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const auto session = Authorize(request, tokens_);
    if (!session) {
        return Fail(request, userver::server::http::HttpStatus::kUnauthorized, "Authentication required");
    }

    std::string parse_error;
    const auto body = ParseBody(request, parse_error);
    if (!body) return ErrorBody(parse_error);

    try {
        const auto hotel_id = ParseOid((*body)["hotel_id"].As<std::string>());
        const auto check_in = ParseDate((*body)["check_in"].As<std::string>());
        const auto check_out = ParseDate((*body)["check_out"].As<std::string>());
        const auto guests = (*body)["guests"].As<int>();
        if (!hotel_id || !check_in || !check_out || *check_out <= *check_in || guests < 1 || guests > 10) {
            return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid booking payload");
        }

        auto hotels = pool_->GetCollection("hotels");
        auto bookings = pool_->GetCollection("bookings");
        const auto hotel = hotels.FindOne(MakeDoc("_id", *hotel_id));
        if (!hotel) {
            return Fail(request, userver::server::http::HttpStatus::kNotFound, "Hotel not found");
        }

        const auto active_count = bookings.Count(MakeDoc(
            "hotel_id", *hotel_id,
            "status", "active",
            "stay.check_in", MakeDoc("$lt", *check_out),
            "stay.check_out", MakeDoc("$gt", *check_in)
        ));
        if (active_count >= static_cast<std::size_t>((*hotel)["rooms_total"].As<int>())) {
            return Fail(request, userver::server::http::HttpStatus::kConflict, "No available rooms for selected dates");
        }

        const auto booking_id = Oid{};
        const auto now = Now();
        bookings.InsertOne(MakeDoc(
            "_id", booking_id,
            "user_id", Oid{session->user_id},
            "hotel_id", *hotel_id,
            "hotel_snapshot", MakeDoc(
                "name", (*hotel)["name"].As<std::string>(),
                "city", (*hotel)["city"].As<std::string>(),
                "address", (*hotel)["address"].As<std::string>(),
                "stars", (*hotel)["stars"].As<int>()
            ),
            "stay", MakeDoc(
                "check_in", *check_in,
                "check_out", *check_out,
                "guests", guests
            ),
            "status", "active",
            "created_at", now,
            "cancelled_at", nullptr,
            "status_history", MakeArray(
                MakeDoc("status", "active", "changed_at", now, "comment", "booking created")
            )
        ));

        const auto created = bookings.FindOne(MakeDoc("_id", booking_id));
        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return ToJson(BookingJson(*created));
    } catch (const std::exception&) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid booking payload");
    }
}

ListUserBookingsHandler::ListUserBookingsHandler(const userver::components::ComponentConfig& config,
                                                 const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pool_(context.FindComponent<userver::components::Mongo>("mongo-db").GetPool()),
      tokens_(context.FindComponent<TokenStore>()) {}

std::string ListUserBookingsHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                                   userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const auto session = Authorize(request, tokens_);
    if (!session) {
        return Fail(request, userver::server::http::HttpStatus::kUnauthorized, "Authentication required");
    }

    const auto user_id_text = request.GetPathArg("user_id");
    if (session->user_id != user_id_text) {
        return Fail(request, userver::server::http::HttpStatus::kForbidden, "Access denied: you can view only your own bookings");
    }

    const auto user_id = ParseOid(user_id_text);
    if (!user_id) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid user_id");
    }

    auto bookings = pool_->GetCollection("bookings");
    auto docs = Collect(bookings.Find(MakeDoc("user_id", *user_id)));
    std::sort(docs.begin(), docs.end(), [](const auto& lhs, const auto& rhs) {
        return lhs["created_at"].template As<std::chrono::system_clock::time_point>() > rhs["created_at"].template As<std::chrono::system_clock::time_point>();
    });

    userver::formats::json::ValueBuilder array(userver::formats::common::Type::kArray);
    for (const auto& doc : docs) array.PushBack(BookingJson(doc));
    return userver::formats::json::ToString(array.ExtractValue());
}

CancelBookingHandler::CancelBookingHandler(const userver::components::ComponentConfig& config,
                                           const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pool_(context.FindComponent<userver::components::Mongo>("mongo-db").GetPool()),
      tokens_(context.FindComponent<TokenStore>()) {}

std::string CancelBookingHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                                userver::server::request::RequestContext&) const {
    PrepareJson(request);
    const auto session = Authorize(request, tokens_);
    if (!session) {
        return Fail(request, userver::server::http::HttpStatus::kUnauthorized, "Authentication required");
    }

    const auto booking_id = ParseOid(request.GetPathArg("booking_id"));
    if (!booking_id) {
        return Fail(request, userver::server::http::HttpStatus::kBadRequest, "Invalid booking_id");
    }

    auto bookings = pool_->GetCollection("bookings");
    const auto current = bookings.FindOne(MakeDoc("_id", *booking_id));
    if (!current) {
        return Fail(request, userver::server::http::HttpStatus::kNotFound, "Booking not found");
    }
    if ((*current)["user_id"].As<Oid>().ToString() != session->user_id) {
        return Fail(request, userver::server::http::HttpStatus::kForbidden, "Access denied: you can cancel only your own booking");
    }
    if ((*current)["status"].As<std::string>() == "cancelled") {
        return Fail(request, userver::server::http::HttpStatus::kConflict, "Booking already cancelled");
    }

    const auto cancelled_at = Now();
    bookings.UpdateOne(
        MakeDoc("_id", *booking_id),
        MakeDoc(
            "$set", MakeDoc("status", "cancelled", "cancelled_at", cancelled_at),
            "$push", MakeDoc("status_history", MakeDoc("status", "cancelled", "changed_at", cancelled_at, "comment", "cancelled by user"))
        )
    );
    const auto updated = bookings.FindOne(MakeDoc("_id", *booking_id));
    return ToJson(BookingJson(*updated));
}

}  // namespace hotel_booking_mongo
