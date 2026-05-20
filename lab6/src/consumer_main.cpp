#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/urabbitmq/component.hpp>
#include <userver/utils/daemon_run.hpp>

#include "event_consumer.hpp"
#include "rabbit_topology.hpp"

int main(int argc, char* argv[]) {
    const auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::clients::dns::Component>()
        .Append<userver::components::Secdist>()
        .Append<userver::components::DefaultSecdistProvider>()
        .Append<userver::components::TestsuiteSupport>()
        .Append<userver::components::Postgres>("postgres-db")
        .Append<userver::components::RabbitMQ>("rabbit-events")
        .Append<hotel_booking_hw06::RabbitTopology>()
        .Append<hotel_booking_hw06::AuditEventConsumer>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
