# Домашнее задание 04
## Система бронирования отелей на MongoDB и userver

Вариант 13. В этой версии API перенесён с Python/FastAPI на C++ и `userver`.

## Что в папке

- `schema_design.md` — документная модель;
- `data.js` — тестовые данные;
- `validation.js` — создание коллекций, валидация, индексы;
- `queries.js` — примеры CRUD-запросов;
- `src/` — HTTP API на `userver`;
- `static_config.yaml` — конфигурация сервиса;
- `Dockerfile`, `docker-compose.yml` — запуск MongoDB и API.

## Эндпоинты

- `POST /auth/register`
- `POST /auth/login`
- `GET /users/by-login/{login}`
- `GET /users/search?mask=...`
- `POST /hotels`
- `GET /hotels`
- `GET /hotels/search?city=...`
- `POST /bookings`
- `GET /users/{user_id}/bookings`
- `PATCH /bookings/{booking_id}/cancel`

## Запуск

```bash
docker compose up --build
```

После старта:

- API: `http://localhost:8000`
- MongoDB: `mongodb://root:root@localhost:27017`

## Проверка

Регистрация:

```bash
curl -X POST http://localhost:8000/auth/register \
  -H 'Content-Type: application/json' \
  -d '{
    "login":"test_user_1",
    "password":"travel123",
    "first_name":"Kirill",
    "last_name":"Sokolov"
  }'
```

Логин:

```bash
curl -X POST http://localhost:8000/auth/login \
  -H 'Content-Type: application/json' \
  -d '{
    "login":"test_user_1",
    "password":"travel123"
  }'
```
