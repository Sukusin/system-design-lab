# Оптимизация запросов и индексы

Ниже приведены те запросы, которые в этой системе будут встречаться чаще всего:
- поиск пользователя по логину;
- поиск пользователя по маске имени и фамилии;
- поиск отелей по городу;
- получение бронирований пользователя;
- проверка доступности номеров при создании бронирования.

Сразу после загрузки тестовых данных полезно обновить статистику:

```sql
ANALYZE users;
ANALYZE hotels;
ANALYZE bookings;
```

## Какие индексы были добавлены

### 1. `idx_hotels_created_by`
```sql
CREATE INDEX idx_hotels_created_by ON hotels(created_by);
```
Нужен для внешнего ключа `hotels.created_by`, а также для возможных выборок отелей, созданных конкретным пользователем.

### 2. `idx_bookings_hotel_id`
```sql
CREATE INDEX idx_bookings_hotel_id ON bookings(hotel_id);
```
Нужен для внешнего ключа `bookings.hotel_id` и для обычных выборок бронирований по отелю.

### 3. `idx_bookings_user_id_created_at`
```sql
CREATE INDEX idx_bookings_user_id_created_at ON bookings(user_id, created_at DESC);
```
Нужен для запроса получения бронирований пользователя. Сразу покрывает фильтрацию по `user_id` и сортировку по дате создания.

### 4. `idx_users_full_name_trgm`
```sql
CREATE INDEX idx_users_full_name_trgm
ON users USING GIN (lower(first_name || ' ' || last_name) gin_trgm_ops);
```
Нужен для поиска по маске имени и фамилии. Обычный B-tree индекс здесь мало помогает, потому что поиск идёт по шаблону вида `%mask%`.

### 5. `idx_hotels_city_trgm`
```sql
CREATE INDEX idx_hotels_city_trgm
ON hotels USING GIN (lower(city) gin_trgm_ops);
```
Нужен для поиска по городу с шаблоном `%city%`.

### 6. `idx_bookings_active_stay_period`
```sql
CREATE INDEX idx_bookings_active_stay_period
ON bookings USING GIST (hotel_id, stay_period)
WHERE status = 'active';
```
Главный индекс для проверки доступности номеров. Он ускоряет поиск только по активным бронированиям и использует диапазон дат вместо ручной проверки четырьмя сравнениями.

---

## 1. Поиск пользователя по маске имени и фамилии

### Запрос
```sql
EXPLAIN
SELECT id, login, first_name, last_name, created_at
FROM users
WHERE lower(first_name || ' ' || last_name) LIKE '%' || lower('sokol') || '%'
ORDER BY id ASC;
```

### До оптимизации
Типичный план:
```text
Seq Scan on users
  Filter: (lower(((first_name || ' '::text) || last_name)) ~~ '%sokol%'::text)
```

### После оптимизации
Типичный план:
```text
Bitmap Heap Scan on users
  Recheck Cond: (lower(((first_name || ' '::text) || last_name)) ~~ '%sokol%'::text)
  -> Bitmap Index Scan on idx_users_full_name_trgm
```

### Что изменилось
Без индекса PostgreSQL вынужден читать всю таблицу `users`. После добавления trigram-индекса он сначала находит подходящие строки по индексу, а потом читает только нужные записи.

---

## 2. Поиск отелей по городу

### Запрос
```sql
EXPLAIN
SELECT id, name, city, address, stars, rooms_total, created_by, created_at
FROM hotels
WHERE lower(city) LIKE '%' || lower('moscow') || '%'
ORDER BY id ASC;
```

### До оптимизации
Типичный план:
```text
Seq Scan on hotels
  Filter: (lower(city) ~~ '%moscow%'::text)
```

### После оптимизации
Типичный план:
```text
Bitmap Heap Scan on hotels
  Recheck Cond: (lower(city) ~~ '%moscow%'::text)
  -> Bitmap Index Scan on idx_hotels_city_trgm
```

### Что изменилось
Поиск по городу перестаёт быть полным обходом таблицы. Это особенно полезно, если количество отелей большое и пользователь часто фильтрует их по городу.

---

## 3. Проверка доступности номеров при создании бронирования

### Запрос
```sql
EXPLAIN
SELECT COUNT(*) AS active_bookings
FROM bookings
WHERE hotel_id = 2
  AND status = 'active'
  AND stay_period && daterange('2026-04-12'::date, '2026-04-15'::date, '[)');
```

### До оптимизации
Типичный план:
```text
Aggregate
  -> Seq Scan on bookings
       Filter: ((hotel_id = 2)
                AND (status = 'active'::booking_status)
                AND (stay_period && '[2026-04-12,2026-04-15)'::daterange))
```

### После оптимизации
Типичный план:
```text
Aggregate
  -> Bitmap Heap Scan on bookings
       Recheck Cond: ((hotel_id = 2)
                      AND (stay_period && '[2026-04-12,2026-04-15)'::daterange))
       Filter: (status = 'active'::booking_status)
       -> Bitmap Index Scan on idx_bookings_active_stay_period
```

### Что изменилось
Проверка пересечения дат становится индексируемой. Это самый важный момент для бронирований, потому что именно этот запрос вызывается при каждом создании новой записи.

---

## 4. Получение бронирований пользователя

### Запрос
```sql
EXPLAIN
SELECT id, user_id, hotel_id, check_in, check_out, guests, status, created_at, cancelled_at
FROM bookings
WHERE user_id = 1
ORDER BY created_at DESC;
```

### После добавления индекса
Ожидаемый план:
```text
Index Scan using idx_bookings_user_id_created_at on bookings
  Index Cond: (user_id = 1)
```

### Что изменилось
PostgreSQL может сразу взять нужные строки в правильном порядке, не делая отдельную сортировку.

---

## Важная оговорка

В учебной базе в каждой таблице всего по 10 записей. На таком маленьком объёме PostgreSQL может всё равно выбрать `Seq Scan`, даже если индекс создан. Это нормальное поведение оптимизатора: для маленьких таблиц полный проход действительно бывает дешевле.

То есть смысл индексов здесь не в том, чтобы обязательно увидеть другой план на 10 строках, а в том, чтобы правильно подготовить схему к росту данных.

## Возможное партиционирование

Для этой системы естественный кандидат на партиционирование — таблица `bookings`.

### Идея
Разбивать бронирования по месяцу `check_in`, например:
- `bookings_2026_04`
- `bookings_2026_05`
- `bookings_2026_06`

