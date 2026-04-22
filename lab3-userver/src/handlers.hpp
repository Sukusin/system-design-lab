#pragma once

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/postgres/cluster.hpp>

#include "token_store.hpp"

namespace hotel_booking_pg {

class RootHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-root";
    using HttpHandlerBase::HttpHandlerBase;
    std::string HandleRequest(userver::server::http::HttpRequest&, userver::server::request::RequestContext&) const override;
};

class RegisterUserHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-register-user";
    RegisterUserHandler(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);
    std::string HandleRequest(userver::server::http::HttpRequest&, userver::server::request::RequestContext&) const override;
private:
    userver::storages::postgres::ClusterPtr pg_;
};

class LoginHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-login";
    LoginHandler(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);
    std::string HandleRequest(userver::server::http::HttpRequest&, userver::server::request::RequestContext&) const override;
private:
    userver::storages::postgres::ClusterPtr pg_;
    TokenStore& tokens_;
};

class GetUserByLoginHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-user-by-login";
    GetUserByLoginHandler(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);
    std::string HandleRequest(userver::server::http::HttpRequest&, userver::server::request::RequestContext&) const override;
private:
    userver::storages::postgres::ClusterPtr pg_;
};

class SearchUsersHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-user-search";
    SearchUsersHandler(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);
    std::string HandleRequest(userver::server::http::HttpRequest&, userver::server::request::RequestContext&) const override;
private:
    userver::storages::postgres::ClusterPtr pg_;
};

class CreateHotelHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-create-hotel";
    CreateHotelHandler(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);
    std::string HandleRequest(userver::server::http::HttpRequest&, userver::server::request::RequestContext&) const override;
private:
    userver::storages::postgres::ClusterPtr pg_;
    TokenStore& tokens_;
};

class ListHotelsHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-list-hotels";
    ListHotelsHandler(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);
    std::string HandleRequest(userver::server::http::HttpRequest&, userver::server::request::RequestContext&) const override;
private:
    userver::storages::postgres::ClusterPtr pg_;
};

class SearchHotelsHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-search-hotels";
    SearchHotelsHandler(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);
    std::string HandleRequest(userver::server::http::HttpRequest&, userver::server::request::RequestContext&) const override;
private:
    userver::storages::postgres::ClusterPtr pg_;
};

class CreateBookingHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-create-booking";
    CreateBookingHandler(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);
    std::string HandleRequest(userver::server::http::HttpRequest&, userver::server::request::RequestContext&) const override;
private:
    userver::storages::postgres::ClusterPtr pg_;
    TokenStore& tokens_;
};

class ListUserBookingsHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-user-bookings";
    ListUserBookingsHandler(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);
    std::string HandleRequest(userver::server::http::HttpRequest&, userver::server::request::RequestContext&) const override;
private:
    userver::storages::postgres::ClusterPtr pg_;
    TokenStore& tokens_;
};

class CancelBookingHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-cancel-booking";
    CancelBookingHandler(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);
    std::string HandleRequest(userver::server::http::HttpRequest&, userver::server::request::RequestContext&) const override;
private:
    userver::storages::postgres::ClusterPtr pg_;
    TokenStore& tokens_;
};

}  // namespace hotel_booking_pg
