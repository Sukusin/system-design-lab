-- Проверка журнала событий
SELECT event_type, aggregate_type, aggregate_id, processed_at
FROM event_log
ORDER BY processed_at DESC
LIMIT 20;

-- Проверка CQRS read model
SELECT booking_id, user_id, hotel_id, status, updated_at
FROM booking_read_model
ORDER BY updated_at DESC
LIMIT 20;

-- Проверка событий по одному бронированию
SELECT event_type, payload, processed_at
FROM event_log
WHERE aggregate_type = 'Booking' AND aggregate_id = 1
ORDER BY processed_at;
