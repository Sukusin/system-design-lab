#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

namespace hotel_booking_mongo {

inline void PrepareJson(userver::server::http::HttpRequest& request) {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
}

inline std::string ErrorBody(std::string_view detail) {
    userver::formats::json::ValueBuilder builder;
    builder["detail"] = std::string(detail);
    return userver::formats::json::ToString(builder.ExtractValue());
}

inline std::string Fail(userver::server::http::HttpRequest& request,
                        userver::server::http::HttpStatus status,
                        std::string_view detail) {
    PrepareJson(request);
    request.SetResponseStatus(status);
    return ErrorBody(detail);
}

inline std::optional<userver::formats::json::Value> ParseBody(
    userver::server::http::HttpRequest& request,
    std::string& error_text
) {
    try {
        return userver::formats::json::FromString(request.RequestBody());
    } catch (const std::exception&) {
        error_text = "Invalid JSON body";
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return std::nullopt;
    }
}

inline std::string GetBearerToken(const userver::server::http::HttpRequest& request) {
    const auto header = request.GetHeader("Authorization");
    constexpr std::string_view kPrefix = "Bearer ";
    if (header.size() <= kPrefix.size() || header.substr(0, kPrefix.size()) != kPrefix) return {};
    return header.substr(kPrefix.size());
}

inline bool IsIsoDate(std::string_view value) {
    if (value.size() != 10) return false;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (i == 4 || i == 7) {
            if (value[i] != '-') return false;
        } else if (value[i] < '0' || value[i] > '9') {
            return false;
        }
    }
    return true;
}

}  // namespace hotel_booking_mongo
