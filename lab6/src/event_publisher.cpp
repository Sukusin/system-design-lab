#include "event_publisher.hpp"

#include <chrono>
#include <string>

#include <userver/engine/deadline.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/urabbitmq/component.hpp>
#include <userver/urabbitmq/typedefs.hpp>
#include <userver/utils/datetime.hpp>

#include "rabbit_topology.hpp"
#include "security.hpp"

namespace hotel_booking_hw06 {

namespace {

userver::engine::Deadline PublishDeadline() {
    return userver::engine::Deadline::FromDuration(std::chrono::seconds{5});
}

}  // namespace

EventPublisher::EventPublisher(const userver::components::ComponentConfig& config,
                               const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      rabbit_(context.FindComponent<userver::components::RabbitMQ>("rabbit-events").GetClient()) {
    context.FindComponent<RabbitTopology>();
}

void EventPublisher::Publish(std::string_view event_type,
                             std::string_view aggregate_type,
                             std::int64_t aggregate_id,
                             std::string_view routing_key,
                             const userver::formats::json::Value& payload) const {
    userver::formats::json::ValueBuilder event;
    event["event_id"] = hotel_booking_pg::GenerateToken();
    event["event_type"] = std::string(event_type);
    event["aggregate_type"] = std::string(aggregate_type);
    event["aggregate_id"] = aggregate_id;
    event["occurred_at"] = userver::utils::datetime::TimestampToString(userver::utils::datetime::Timestamp());
    event["payload"] = payload;

    rabbit_->PublishReliable(
        userver::urabbitmq::Exchange{std::string{kEventsExchange}},
        std::string(routing_key),
        userver::formats::json::ToString(event.ExtractValue()),
        PublishDeadline()
    );
}

}  // namespace hotel_booking_hw06
