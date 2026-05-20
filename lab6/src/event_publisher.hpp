#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/urabbitmq/client.hpp>

namespace hotel_booking_hw06 {

class EventPublisher final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "event-publisher";

    EventPublisher(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

    void Publish(std::string_view event_type,
                 std::string_view aggregate_type,
                 std::int64_t aggregate_id,
                 std::string_view routing_key,
                 const userver::formats::json::Value& payload) const;

private:
    std::shared_ptr<userver::urabbitmq::Client> rabbit_;
};

}  // namespace hotel_booking_hw06
