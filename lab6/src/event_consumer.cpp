#include "event_consumer.hpp"

#include <cstdint>
#include <string>

#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/component.hpp>

namespace hotel_booking_hw06 {

AuditEventConsumer::AuditEventConsumer(const userver::components::ComponentConfig& config,
                                       const userver::components::ComponentContext& context)
    : ConsumerComponentBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()) {
    context.FindComponent<RabbitTopology>();
}

void AuditEventConsumer::Process(std::string message) {
    const auto event = userver::formats::json::FromString(message);

    const auto event_id = event["event_id"].As<std::string>();
    const auto event_type = event["event_type"].As<std::string>();
    const auto aggregate_type = event["aggregate_type"].As<std::string>();
    const auto aggregate_id = event["aggregate_id"].As<std::int64_t>();
    const auto occurred_at = event["occurred_at"].As<std::string>();
    const auto payload = event["payload"];

    pg_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                 R"(
                     INSERT INTO event_log (
                         event_id,
                         event_type,
                         aggregate_type,
                         aggregate_id,
                         occurred_at,
                         payload
                     )
                     VALUES ($1, $2, $3, $4, $5::timestamptz, $6::jsonb)
                     ON CONFLICT (event_id) DO NOTHING
                 )",
                 event_id,
                 event_type,
                 aggregate_type,
                 aggregate_id,
                 occurred_at,
                 userver::formats::json::ToString(payload));

    if (event_type == "BookingCreated" || event_type == "BookingCancelled") {
        const auto booking_id = payload["booking_id"].As<std::int64_t>();
        const auto user_id = payload["user_id"].As<std::int64_t>();
        const auto hotel_id = payload["hotel_id"].As<std::int64_t>();
        const auto status = payload["status"].As<std::string>();

        pg_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                     R"(
                         INSERT INTO booking_read_model (
                             booking_id,
                             user_id,
                             hotel_id,
                             status,
                             last_event_id,
                             updated_at
                         )
                         VALUES ($1, $2, $3, $4, $5, NOW())
                         ON CONFLICT (booking_id) DO UPDATE
                         SET status = EXCLUDED.status,
                             last_event_id = EXCLUDED.last_event_id,
                             updated_at = NOW()
                     )",
                     booking_id,
                     user_id,
                     hotel_id,
                     status,
                     event_id);
    }

    LOG_INFO() << "Processed event " << event_type << " id=" << event_id;
}

}  // namespace hotel_booking_hw06
