-- 1. Создание нового пользователя
INSERT INTO users (login, password_hash, first_name, last_name)
VALUES ('new_user', 'salt$hash', 'Ivan', 'Petrov')
RETURNING id, login, first_name, last_name, created_at;

-- 2. Поиск пользователя по логину
SELECT id, login, first_name, last_name, created_at
FROM users
WHERE login = 'kirill_sokolov';

-- 3. Поиск пользователя по маске имени и фамилии
SELECT id, login, first_name, last_name, created_at
FROM users
WHERE lower(first_name || ' ' || last_name) LIKE '%' || lower('sokol') || '%'
ORDER BY id ASC;

-- 4. Создание отеля
INSERT INTO hotels (name, city, address, stars, rooms_total, created_by)
VALUES ('Central Hotel', 'Moscow', 'Tverskaya 10', 4, 35, 1)
RETURNING id, name, city, address, stars, rooms_total, created_by, created_at;

-- 5. Получение списка отелей
SELECT id, name, city, address, stars, rooms_total, created_by, created_at
FROM hotels
ORDER BY id ASC;

-- 6. Поиск отелей по городу
SELECT id, name, city, address, stars, rooms_total, created_by, created_at
FROM hotels
WHERE lower(city) LIKE '%' || lower('moscow') || '%'
ORDER BY id ASC;

-- 7. Проверка количества активных бронирований на пересекающийся период
SELECT COUNT(*) AS active_bookings
FROM bookings
WHERE hotel_id = 2
  AND status = 'active'
  AND stay_period && daterange('2026-04-12'::date, '2026-04-15'::date, '[)');

-- 8. Создание бронирования
INSERT INTO bookings (user_id, hotel_id, check_in, check_out, guests)
VALUES (1, 2, '2026-04-12', '2026-04-15', 2)
RETURNING id, user_id, hotel_id, check_in, check_out, guests, status, created_at, cancelled_at;

-- 9. Получение бронирований пользователя
SELECT id, user_id, hotel_id, check_in, check_out, guests, status, created_at, cancelled_at
FROM bookings
WHERE user_id = 1
ORDER BY created_at DESC;

-- 10. Отмена бронирования
UPDATE bookings
SET status = 'cancelled',
    cancelled_at = NOW()
WHERE id = 1
  AND user_id = 1
  AND status = 'active'
RETURNING id, user_id, hotel_id, check_in, check_out, guests, status, created_at, cancelled_at;
