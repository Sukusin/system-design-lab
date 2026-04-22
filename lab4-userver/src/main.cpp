#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include "handlers.hpp"
#include "token_store.hpp"

int main(int argc, char* argv[]) {
    const auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::clients::dns::Component>()
        .Append<userver::components::Mongo>("mongo-db")
        .Append<hotel_booking_mongo::TokenStore>()
        .Append<hotel_booking_mongo::RootHandler>()
        .Append<hotel_booking_mongo::RegisterUserHandler>()
        .Append<hotel_booking_mongo::LoginHandler>()
        .Append<hotel_booking_mongo::GetUserByLoginHandler>()
        .Append<hotel_booking_mongo::SearchUsersHandler>()
        .Append<hotel_booking_mongo::CreateHotelHandler>()
        .Append<hotel_booking_mongo::ListHotelsHandler>()
        .Append<hotel_booking_mongo::SearchHotelsHandler>()
        .Append<hotel_booking_mongo::CreateBookingHandler>()
        .Append<hotel_booking_mongo::ListUserBookingsHandler>()
        .Append<userver::components::TestsuiteSupport>()
        .Append<hotel_booking_mongo::CancelBookingHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
