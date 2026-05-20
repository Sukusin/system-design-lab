# Домашнее задание 06: Event-Driven архитектура для системы бронирования отелей

## 1. Цель

Цель работы — спроектировать и реализовать событийно-ориентированную архитектуру для системы бронирования отелей. В системе есть пользователи, отели и бронирования. Командная часть API изменяет состояние в PostgreSQL и публикует события в RabbitMQ. Отдельный consumer получает события и обновляет read model.

## 2. Основные команды системы

Команды — это действия пользователя или администратора, которые изменяют состояние системы.

| Команда | HTTP endpoint | Что делает | Событие |
|---|---|---|---|
| CreateUser | `POST /auth/register` | Создаёт нового пользователя | `UserCreated` |
| CreateHotel | `POST /hotels` | Создаёт новый отель | `HotelCreated` |
| CreateBooking | `POST /bookings` | Создаёт бронирование | `BookingCreated` |
| CancelBooking | `PATCH /bookings/{booking_id}/cancel` | Отменяет бронирование | `BookingCancelled` |

Запросы чтения (`GET /hotels`, `GET /hotels/search`, `GET /users/{id}/bookings`) не публикуют события, потому что они не меняют состояние системы.

## 3. События системы

Событие — это факт, который уже произошёл в системе. Названия событий пишутся в прошедшем времени:

- `UserCreated`
- `HotelCreated`
- `BookingCreated`
- `BookingCancelled`

Общий формат события:

```json
{
  "event_id": "unique-event-id",
  "event_type": "BookingCreated",
  "aggregate_type": "Booking",
  "aggregate_id": 123,
  "occurred_at": "2026-05-06T11:43:05+0000",
  "payload": {
    "booking_id": 123,
    "user_id": 11,
    "hotel_id": 5,
    "check_in": "2026-09-10",
    "check_out": "2026-09-15",
    "guests": 2,
    "status": "active"
  }
}
```

## 4. Producer и consumer

### Producer

Producer находится внутри REST API на userver. После успешного изменения PostgreSQL handler публикует событие в RabbitMQ через компонент `components::RabbitMQ`.

Producer публикует события в exchange:

```text
hotel.events
```

Тип exchange:

```text
topic
```

Используемые routing keys:

```text
user.created
hotel.created
booking.created
booking.cancelled
```

### Consumer

Consumer — отдельный userver-сервис. Он читает сообщения из очереди:

```text
hotel.events.audit
```

Очередь привязана к exchange `hotel.events` по routing keys:

```text
user.*
hotel.*
booking.*
```

Consumer обрабатывает событие и пишет его в таблицу `event_log`. Для событий бронирования он дополнительно обновляет CQRS read model `booking_read_model`.

## 5. RabbitMQ routing

Выбран RabbitMQ, потому что для учебной работы он проще Kafka: легко поднять в Docker, удобно смотреть exchange/queue/messages через Management UI.

Схема маршрутизации:

```text
API command handler
        |
        v
RabbitMQ exchange: hotel.events, type=topic
        |
        +-- routing key user.created       -> queue hotel.events.audit
        +-- routing key hotel.created      -> queue hotel.events.audit
        +-- routing key booking.created    -> queue hotel.events.audit
        +-- routing key booking.cancelled  -> queue hotel.events.audit
        |
        v
Consumer: AuditEventConsumer
        |
        +-- event_log
        +-- booking_read_model
```

## 6. Гарантии доставки

В проекте используется `PublishReliable`, то есть producer ждёт подтверждение публикации от RabbitMQ. Consumer реализован через `urabbitmq::ConsumerComponentBase`.

Гарантия доставки: **at-least-once**.

Это значит, что событие может быть доставлено повторно. Поэтому consumer должен быть идемпотентным. Для этого:

- `event_log.event_id` является PRIMARY KEY;
- вставка события делается через `ON CONFLICT (event_id) DO NOTHING`;
- read model обновляется через `ON CONFLICT (booking_id) DO UPDATE`.

## 7. CQRS

CQRS применим, потому что операции записи и чтения имеют разные требования.

### Write model

Write model — основные таблицы PostgreSQL:

- `users`
- `hotels`
- `bookings`

Команды работают именно с этой моделью.

### Read model

Read model — отдельная таблица:

```text
booking_read_model
```

Она обновляется consumer'ом на основе событий:

- `BookingCreated` создаёт или обновляет строку бронирования со статусом `active`;
- `BookingCancelled` обновляет статус бронирования на `cancelled`.

В реальной системе таких read model могло бы быть больше: например, счётчик активных бронирований пользователя, история уведомлений, витрина бронирований для личного кабинета.

## 8. Почему это улучшает архитектуру

Событийная архитектура позволяет отделить основную бизнес-операцию от побочных действий. Например, при создании бронирования API не обязан напрямую отправлять письмо, обновлять аналитическую таблицу и уведомлять другие сервисы. Он только публикует факт `BookingCreated`, а остальные сервисы реагируют на него независимо.

Плюсы:

- слабая связность между сервисами;
- проще добавлять новые consumer'ы;
- можно строить read model асинхронно;
- можно повторно обработать события;
- система лучше масштабируется по чтению и фоновым задачам.

Минусы:

- появляется eventual consistency;
- нужна идемпотентность consumer'ов;
- сложнее отлаживать поток данных;
- нужен мониторинг очередей и dead-letter сценарии.

## 9. Что реализовано в коде

В проекте реализовано:

- API на C++/userver;
- PostgreSQL как основная write model;
- RabbitMQ как брокер сообщений;
- producer в API handlers;
- отдельный userver consumer;
- таблица `event_log`;
- CQRS read model `booking_read_model`;
- Docker Compose для запуска PostgreSQL, RabbitMQ, API и consumer.
