# Домашнее задание 03
## Система бронирования отелей на PostgreSQL и userver

Вариант 13. В этой версии API перенесён с Python/FastAPI на C++ и `userver`.

## Что в папке

- `schema.sql` — схема PostgreSQL;
- `data.sql` — тестовые данные;
- `queries.sql` — запросы для операций из задания;
- `optimization.md` — краткое описание индексов и проверок через `EXPLAIN`;
- `src/` — HTTP API на `userver`;
- `static_config.yaml` — конфигурация сервиса;
- `Dockerfile`, `docker-compose.yaml` — запуск PostgreSQL и API.

## Что реализовано в API

- `POST /auth/register` — создание пользователя;
- `POST /auth/login` — вход и получение bearer token;
- `GET /users/by-login/{login}` — поиск по логину;
- `GET /users/search?mask=...` — поиск по маске имени и фамилии;
- `POST /hotels` — создание отеля;
- `GET /hotels` — список отелей;
- `GET /hotels/search?city=...` — поиск отелей по городу;
- `POST /bookings` — создание бронирования;
- `GET /users/{user_id}/bookings` — бронирования пользователя;
- `PATCH /bookings/{booking_id}/cancel` — отмена бронирования.

## Запуск

```bash
docker compose up --build
```

После старта:

- API: `http://localhost:8000`
- PostgreSQL: `localhost:5432`

## Проверка

Примеры запросов можно отправлять любым HTTP-клиентом.

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

Вход:

```bash
curl -X POST http://localhost:8000/auth/login \
  -H 'Content-Type: application/json' \
  -d '{
    "login":"test_user_1",
    "password":"travel123"
  }'
```

Дальше токен нужно передавать в заголовке:

```text
Authorization: Bearer <token>
```

Создание отеля:

```bash
curl -X POST http://localhost:8000/hotels \
  -H 'Content-Type: application/json' \
  -H 'Authorization: Bearer <token>' \
  -d '{
    "name":"Central Park Hotel",
    "city":"Moscow",
    "address":"Polevaya 5",
    "stars":4,
    "rooms_total":25
  }'
```
