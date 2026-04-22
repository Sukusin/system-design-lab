# Домашнее задание 04 — MongoDB

## Тема

Вариант 13: **система бронирования отелей**.

В проекте реализованы:

- MongoDB как основная БД;
- коллекции `users`, `hotels`, `bookings`;
- валидация схем через `$jsonSchema`;
- тестовые данные;
- CRUD-запросы для всех операций по варианту;
- API на FastAPI для работы с MongoDB.

---

## Структура проекта

```text
.
├── app/
│   ├── auth.py
│   ├── database.py
│   ├── main.py
│   └── schemas.py
├── data.js
├── validation.js
├── queries.js
├── schema_design.md
├── Dockerfile
├── docker-compose.yml
├── requirements.txt
└── README.md
```

---

## Что где находится

- `schema_design.md` — описание документной модели и обоснование embedded/references;
- `data.js` — тестовые данные;
- `queries.js` — примеры CRUD-запросов и aggregation pipeline;
- `validation.js` — создание коллекций, валидация и индексы;
- `app/` — FastAPI API с подключением к MongoDB;
- `docker-compose.yml` — запуск MongoDB и API.

---

## Модель данных

### Коллекции

- `users`
- `hotels`
- `bookings`

### Почему так

Основные сущности вынесены в отдельные коллекции, потому что они живут независимо и участвуют в разных запросах. Встроенные документы используются там, где данные принадлежат только одному объекту: например, `stay`, `status_history` и `hotel_snapshot` внутри бронирования.

Подробнее — в `schema_design.md`.

---

## Запуск проекта

### 1. Запустить контейнеры

```bash
 docker compose up --build
```

После старта будут доступны:

- API: `http://localhost:8000`
- Swagger UI: `http://localhost:8000/docs`
- MongoDB: `mongodb://root:root@localhost:27017`

Официальный Docker-образ MongoDB поддерживает запуск инициализационных скриптов из каталога `/docker-entrypoint-initdb.d`, поэтому `validation.js` и `data.js` автоматически применяются при первом создании базы.

---

## Если нужно полностью пересоздать базу

Так как данные хранятся в volume, для полного сброса нужно удалить volume:

```bash
 docker compose down -v
 docker compose up --build
```

---

## Как зайти в MongoDB внутри контейнера

```bash
 docker exec -it hotel-booking-mongo mongosh -u root -p root --authenticationDatabase admin
```

Переключиться на нужную БД:

```javascript
 use hotel_booking_mongo
```

Проверить коллекции:

```javascript
 show collections
 db.users.countDocuments()
 db.hotels.countDocuments()
 db.bookings.countDocuments()
```

---

## Как выполнить запросы из `queries.js`

Проще всего выполнить файл с хоста одной командой:

```bash
 docker exec -i hotel-booking-mongo mongosh -u root -p root --authenticationDatabase admin hotel_booking_mongo < queries.js
```

Либо можно просто открыть `queries.js` и копировать команды в `mongosh` по частям.

---

## Проверка API

### 1. Регистрация пользователя

`POST /auth/register`

```json
{
  "login": "test_user_1",
  "password": "travel123",
  "first_name": "Kirill",
  "last_name": "Sokolov"
}
```

### 2. Логин

`POST /auth/login`

```json
{
  "login": "test_user_1",
  "password": "travel123"
}
```

В ответе придёт `access_token`.

### 3. Авторизация в Swagger

В Swagger нажать **Authorize** и вставить:

```text
Bearer <access_token>
```

### 4. Дальше можно проверить

- `GET /users/by-login/{login}`
- `GET /users/search?mask=Kir`
- `POST /hotels`
- `GET /hotels`
- `GET /hotels/search?city=Amsterdam`
- `POST /bookings`
- `GET /users/{user_id}/bookings`
- `PATCH /bookings/{booking_id}/cancel`

---

## Валидация схем

В MongoDB для валидации коллекций можно использовать `$jsonSchema` через `createCollection` или изменение коллекции командой `collMod`. Это и используется в `validation.js`. citeturn447100search0turn447100search4

### Что валидируется

#### `users`

- обязательные поля;
- длина строки;
- паттерн логина;
- тип `date` для `created_at`.

#### `hotels`

- обязательные поля;
- диапазон `stars` от 1 до 5;
- диапазон `rooms_total`;
- массив `amenities`.

#### `bookings`

- обязательные поля;
- вложенные документы `hotel_snapshot`, `stay`, `status_history`;
- допустимые значения `status`;
- дополнительная проверка, что `check_out > check_in`.

### Как проверить ошибку валидации

В файле `validation.js` есть флаг:

```javascript
const RUN_NEGATIVE_TESTS = false;
```

Если поменять его на `true` и снова выполнить скрипт, будет попытка вставить невалидный документ. MongoDB вернёт ошибку валидации.

---

## Индексы

Создаются индексы:

- `users.login` — уникальный;
- `users.first_name, users.last_name`;
- `hotels.city`;
- `hotels.created_by`;
- `hotels.amenities`;
- `bookings.user_id, created_at`;
- `bookings.hotel_id, status`;
- `bookings.hotel_id, status, stay.check_in, stay.check_out`.

Они ускоряют поиск пользователей, отелей по городу и проверку доступности при создании бронирования.

---

## Пример агрегации

В `queries.js` есть pipeline, который показывает:

- количество активных бронирований по городам;
- среднее число гостей;
- сортировку по убыванию количества бронирований.

Используются стадии:

- `$match`
- `$group`
- `$project`
- `$sort`

---

## Остановка проекта

```bash
 docker compose down
```

Полное удаление контейнеров и данных:

```bash
 docker compose down -v
```

---

