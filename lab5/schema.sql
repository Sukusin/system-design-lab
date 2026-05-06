BEGIN;

CREATE EXTENSION IF NOT EXISTS pg_trgm;
CREATE EXTENSION IF NOT EXISTS btree_gist;

DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'booking_status') THEN
        CREATE TYPE booking_status AS ENUM ('active', 'cancelled');
    END IF;
END
$$;

CREATE TABLE IF NOT EXISTS users (
    id BIGSERIAL PRIMARY KEY,
    login VARCHAR(50) NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    first_name VARCHAR(100) NOT NULL,
    last_name VARCHAR(100) NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT users_login_not_blank CHECK (char_length(trim(login)) >= 3),
    CONSTRAINT users_first_name_not_blank CHECK (char_length(trim(first_name)) >= 1),
    CONSTRAINT users_last_name_not_blank CHECK (char_length(trim(last_name)) >= 1)
);

CREATE TABLE IF NOT EXISTS hotels (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(200) NOT NULL,
    city VARCHAR(120) NOT NULL,
    address TEXT NOT NULL,
    stars SMALLINT NOT NULL,
    rooms_total INTEGER NOT NULL,
    created_by BIGINT NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT hotels_name_not_blank CHECK (char_length(trim(name)) >= 2),
    CONSTRAINT hotels_city_not_blank CHECK (char_length(trim(city)) >= 1),
    CONSTRAINT hotels_address_not_blank CHECK (char_length(trim(address)) >= 5),
    CONSTRAINT hotels_stars_range CHECK (stars BETWEEN 1 AND 5),
    CONSTRAINT hotels_rooms_total_positive CHECK (rooms_total > 0)
);

CREATE TABLE IF NOT EXISTS bookings (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    hotel_id BIGINT NOT NULL REFERENCES hotels(id) ON DELETE RESTRICT,
    check_in DATE NOT NULL,
    check_out DATE NOT NULL,
    guests SMALLINT NOT NULL,
    status booking_status NOT NULL DEFAULT 'active',
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    cancelled_at TIMESTAMPTZ,
    stay_period DATERANGE GENERATED ALWAYS AS (daterange(check_in, check_out, '[)')) STORED,
    CONSTRAINT bookings_dates_order CHECK (check_out > check_in),
    CONSTRAINT bookings_guests_range CHECK (guests BETWEEN 1 AND 10),
    CONSTRAINT bookings_status_cancelled_at CHECK (
        (status = 'active' AND cancelled_at IS NULL)
        OR (status = 'cancelled' AND cancelled_at IS NOT NULL)
    )
);

CREATE INDEX IF NOT EXISTS idx_hotels_created_by ON hotels(created_by);
CREATE INDEX IF NOT EXISTS idx_bookings_hotel_id ON bookings(hotel_id);
CREATE INDEX IF NOT EXISTS idx_bookings_user_id_created_at ON bookings(user_id, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_users_full_name_trgm
    ON users USING GIN (lower(first_name || ' ' || last_name) gin_trgm_ops);

CREATE INDEX IF NOT EXISTS idx_hotels_city_trgm
    ON hotels USING GIN (lower(city) gin_trgm_ops);

CREATE INDEX IF NOT EXISTS idx_bookings_active_stay_period
    ON bookings USING GIST (hotel_id, stay_period)
    WHERE status = 'active';

COMMIT;
