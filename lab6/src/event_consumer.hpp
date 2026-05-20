#pragma once

#include <string_view>

#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/urabbitmq/consumer_component_base.hpp>

#include "rabbit_topology.hpp"

namespace hotel_booking_hw06 {

class AuditEventConsumer final : public userver::urabbitmq::ConsumerComponentBase {
public:
    static constexpr std::string_view kName = "consumer-audit-events";

    AuditEventConsumer(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);

private:
    void Process(std::string message) override;

    userver::storages::postgres::ClusterPtr pg_;
};

}  // namespace hotel_booking_hw06
