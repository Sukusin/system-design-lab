# Hotel Booking REST API

Домашнее задание №2 по **«Архитектура программных систем»**.

Вариант **13 — Система бронирования отелей**.

Реализован REST API сервис на **Python + FastAPI + SQLite + JWT**.

## Что реализовано

### Сущности
- Пользователь
- Отель
- Бронирование

### Реализованные API операции
- Создание нового пользователя
- Поиск пользователя по логину
- Поиск пользователя по маске имени и фамилии
- Создание отеля
- Получение списка отелей
- Поиск отелей по городу
- Создание бронирования
- Получение бронирований пользователя
- Отмена бронирования
- Регистрация / логин пользователя

## Почему это REST
- Для чтения используются `GET`
- Для создания используются `POST`
- Для частичного изменения статуса бронирования используется `PATCH`
- Используются корректные статус-коды: `200`, `201`, `401`, `403`, `404`, `409`, `422`
- Ресурсы оформлены через понятные URL: `/users`, `/hotels`, `/bookings`, `/auth`

## Структура проекта

```text
hotel_booking_api/
├── app/
│   ├── __init__.py
│   ├── auth.py
│   ├── database.py
│   ├── main.py
│   └── schemas.py
├── scripts/
│   └── export_openapi.py
├── tests/
│   ├── cancel_booking.bash
│   ├── check_bookings.bash
│   ├── check_customer_bookings.bash
│   ├── conftest.py
│   ├── create_hotel.bash
│   └── test_api.py
├── Dockerfile
├── docker-compose.yaml
├── openapi.yaml
├── README.md
├── pyproject.toml
├── requirements.txt
└── uv.lock
```

## Проектирование API

### 1. Аутентификация

#### POST `/auth/register`
Создание нового пользователя.

**Request body**
```json
{
  "login": "kirill_sokolov",
  "password": "secret123",
  "first_name": "Kirill",
  "last_name": "Sokolov"
}
```

**Response 201**
```json
{
  "id": 1,
  "login": "kirill_sokolov",
  "first_name": "Kirill",
  "last_name": "Sokolov",
  "created_at": "2026-03-28T12:00:00Z"
}
```

#### POST `/auth/login`
Логин и получение JWT токена.

**Request body**
```json
{
  "login": "kirill_sokolov",
  "password": "secret123"
}
```

**Response 200**
```json
{
  "access_token": "<jwt_token>",
  "token_type": "bearer"
}
```

### 2. Пользователи

#### GET `/users/by-login/{login}`
Поиск пользователя по логину.

#### GET `/users/search?mask=Kir`
Поиск пользователя по маске имени и фамилии.

### 3. Отели

#### POST `/hotels`
Создание отеля. Требует JWT.

**Request body**
```json
{
  "name": "5 Star Hotel",
  "city": "Moscow",
  "address": "Polevaya 5",
  "stars": 5,
  "rooms_total": 25
}
```

#### GET `/hotels`
Получение списка всех отелей.

#### GET `/hotels/search?city=Moscow`
Поиск отелей по городу.

### 4. Бронирования

#### POST `/bookings`
Создание бронирования. Требует JWT.

**Request body**
```json
{
  "hotel_id": 1,
  "check_in": "2026-04-10",
  "check_out": "2026-04-15",
  "guests": 2
}
```

#### GET `/users/{user_id}/bookings`
Получение бронирований пользователя. Пользователь может смотреть только свои бронирования.

#### PATCH `/bookings/{booking_id}/cancel`
Отмена бронирования. Вместо удаления запись переводится в статус `cancelled`.

## Защищенные endpoint'ы
JWT-аутентификацией защищены:
- `POST /hotels`
- `POST /bookings`
- `GET /users/{user_id}/bookings`
- `PATCH /bookings/{booking_id}/cancel`

## Запуск локально

### 1. Установка зависимостей
```bash
python -m venv .venv
source .venv/bin/activate  # Linux/macOS
# или .venv\Scripts\activate для Windows
pip install -r requirements.txt
```
Для uv
```bash
uv venv
source .venv/bin/activate  # Linux/macOS
# или .venv\Scripts\activate для Windows
uv sync
```

### 2. Запуск приложения
```bash
uvicorn app.main:app --reload
```

Приложение будет доступно по адресу:
- API: `http://127.0.0.1:8000`
- Swagger UI: `http://127.0.0.1:8000/docs`
- ReDoc: `http://127.0.0.1:8000/redoc`

## Запуск через Docker

### Сборка и запуск
```bash
docker compose up --build
```

## Генерация OpenAPI YAML
```bash
python scripts/export_openapi.py
```

## Примеры использования API

### Регистрация
```bash
curl -X POST http://127.0.0.1:8000/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "login": "kirill",
    "password": "secret123",
    "first_name": "Kirill",
    "last_name": "Sokolov"
  }'
```

### Логин
```bash
curl -X POST http://127.0.0.1:8000/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "login": "kirill",
    "password": "secret123"
  }'
```

### Создание отеля
```bash
curl -X POST http://127.0.0.1:8000/hotels \
  -H "Authorization: Bearer <TOKEN>" \
  -H "Content-Type: application/json" \
  -d '{
    "name": "5 Start Hote",
    "city": "Moscow",
    "address": "Polevaya 5",
    "stars": 5,
    "rooms_total": 20
  }'
```

### Создание бронирования
```bash
curl -X POST http://127.0.0.1:8000/bookings \
  -H "Authorization: Bearer <TOKEN>" \
  -H "Content-Type: application/json" \
  -d '{
    "hotel_id": 1,
    "check_in": "2026-04-10",
    "check_out": "2026-04-15",
    "guests": 2
  }'
```

### Получение бронирований пользователя
```bash
curl -X GET http://127.0.0.1:8000/users/1/bookings \
  -H "Authorization: Bearer <TOKEN>"
```

### Отмена бронирования
```bash
curl -X PATCH http://127.0.0.1:8000/bookings/1/cancel \
  -H "Authorization: Bearer <TOKEN>"
```

## Тесты
Запуск тестов:
```bash
pytest -v
```

Покрыты сценарии:
- успешная регистрация
- ошибка при дублировании логина
- логин и получение JWT
- защита endpoint'ов без токена
- создание отеля и поиск по городу
- создание бронирования, получение списка и отмена
- ошибка при невалидных датах

## Примечания по архитектуре
- Используется SQLite для простоты запуска
- DTO реализованы через Pydantic
- JWT проверяется через middleware
- Для отмены бронирования используется `PATCH`, а не `DELETE`, так как нужно сохранить историю
- При создании бронирования есть простая проверка доступности комнат на выбранные даты
