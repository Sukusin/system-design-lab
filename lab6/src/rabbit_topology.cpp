#include "rabbit_topology.hpp"

#include <chrono>

#include <userver/engine/deadline.hpp>
#include <userver/urabbitmq/component.hpp>
#include <userver/urabbitmq/typedefs.hpp>
#include <userver/utils/flags.hpp>

namespace hotel_booking_hw06 {

namespace {

userver::engine::Deadline RabbitDeadline() {
    return userver::engine::Deadline::FromDuration(std::chrono::seconds{10});
}

}  // namespace

RabbitTopology::RabbitTopology(const userver::components::ComponentConfig& config,
                               const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      rabbit_(context.FindComponent<userver::components::RabbitMQ>("rabbit-events").GetClient()) {
    const userver::urabbitmq::Exchange exchange{std::string{kEventsExchange}};
    const userver::urabbitmq::Queue queue{std::string{kAuditQueue}};

    rabbit_->DeclareExchange(exchange, userver::urabbitmq::Exchange::Type::kTopic, RabbitDeadline());
    rabbit_->DeclareQueue(queue, RabbitDeadline());

    rabbit_->BindQueue(exchange, queue, "user.*", RabbitDeadline());
    rabbit_->BindQueue(exchange, queue, "hotel.*", RabbitDeadline());
    rabbit_->BindQueue(exchange, queue, "booking.*", RabbitDeadline());
}

}  // namespace hotel_booking_hw06
