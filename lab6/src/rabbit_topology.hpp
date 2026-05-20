#pragma once

#include <memory>
#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/urabbitmq/client.hpp>

namespace hotel_booking_hw06 {

inline constexpr std::string_view kEventsExchange = "hotel.events";
inline constexpr std::string_view kAuditQueue = "hotel.events.audit";

class RabbitTopology final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "rabbit-topology";

    RabbitTopology(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

private:
    std::shared_ptr<userver::urabbitmq::Client> rabbit_;
};

}  // namespace hotel_booking_hw06
