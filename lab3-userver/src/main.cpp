#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include "handlers.hpp"
#include "token_store.hpp"

int main(int argc, char* argv[]) {
    const auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::clients::dns::Component>()
        .Append<userver::components::Postgres>("postgres-db")
        .Append<hotel_booking_pg::TokenStore>()
        .Append<hotel_booking_pg::RootHandler>()
        .Append<hotel_booking_pg::RegisterUserHandler>()
        .Append<hotel_booking_pg::LoginHandler>()
        .Append<hotel_booking_pg::GetUserByLoginHandler>()
        .Append<hotel_booking_pg::SearchUsersHandler>()
        .Append<hotel_booking_pg::CreateHotelHandler>()
        .Append<hotel_booking_pg::ListHotelsHandler>()
        .Append<hotel_booking_pg::SearchHotelsHandler>()
        .Append<hotel_booking_pg::CreateBookingHandler>()
        .Append<hotel_booking_pg::ListUserBookingsHandler>()
        .Append<userver::components::TestsuiteSupport>()
        .Append<hotel_booking_pg::CancelBookingHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
