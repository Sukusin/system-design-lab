# Event Catalog

## Общий формат события

```json
{
  "event_id": "string",
  "event_type": "string",
  "aggregate_type": "string",
  "aggregate_id": 1,
  "occurred_at": "2026-05-06T11:43:05+0000",
  "payload": {}
}
```

## UserCreated

**Когда возникает:** после успешного создания пользователя.

**Команда:** `CreateUser` / `POST /auth/register`

**Producer:** REST API service

**Routing key:** `user.created`

**Consumers:**

- `AuditEventConsumer`
- потенциально Notification Service
- потенциально Analytics Service

**Payload:**

```json
{
  "user_id": 11,
  "login": "kirill_sokolov",
  "first_name": "Kirill",
  "last_name": "Sokolov"
}
```

**Гарантия доставки:** at-least-once.

**Идемпотентность:** `event_log.event_id` защищает от повторной вставки одного и того же события.

---

## HotelCreated

**Когда возникает:** после успешного создания отеля.

**Команда:** `CreateHotel` / `POST /hotels`

**Producer:** REST API service

**Routing key:** `hotel.created`

**Consumers:**

- `AuditEventConsumer`
- потенциально Search Index Service
- потенциально Analytics Service

**Payload:**

```json
{
  "hotel_id": 11,
  "name": "Event Hotel",
  "city": "Moscow",
  "address": "Event Street 10",
  "stars": 5,
  "rooms_total": 30,
  "created_by": 11
}
```

**Гарантия доставки:** at-least-once.

**Идемпотентность:** `event_log.event_id` защищает от повторной вставки события.

---

## BookingCreated

**Когда возникает:** после успешного создания бронирования.

**Команда:** `CreateBooking` / `POST /bookings`

**Producer:** REST API service

**Routing key:** `booking.created`

**Consumers:**

- `AuditEventConsumer`
- потенциально Notification Service
- потенциально Billing Service
- потенциально Analytics Service

**Payload:**

```json
{
  "booking_id": 20,
  "user_id": 11,
  "hotel_id": 5,
  "check_in": "2026-09-10",
  "check_out": "2026-09-15",
  "guests": 2,
  "status": "active"
}
```

**Гарантия доставки:** at-least-once.

**Идемпотентность:**

- `event_log.event_id` защищает журнал событий;
- `booking_read_model.booking_id` обновляется через UPSERT.

**Влияние на CQRS:** создаёт/обновляет запись в `booking_read_model` со статусом `active`.

---

## BookingCancelled

**Когда возникает:** после успешной отмены бронирования.

**Команда:** `CancelBooking` / `PATCH /bookings/{booking_id}/cancel`

**Producer:** REST API service

**Routing key:** `booking.cancelled`

**Consumers:**

- `AuditEventConsumer`
- потенциально Notification Service
- потенциально Billing Service
- потенциально Analytics Service

**Payload:**

```json
{
  "booking_id": 20,
  "user_id": 11,
  "hotel_id": 5,
  "status": "cancelled"
}
```

**Гарантия доставки:** at-least-once.

**Идемпотентность:**

- повтор события не создаёт дубль в `event_log`;
- повторное обновление `booking_read_model` оставляет тот же статус `cancelled`.

**Влияние на CQRS:** обновляет `booking_read_model.status` на `cancelled`.
