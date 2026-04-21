import os
import time
from contextlib import contextmanager

import psycopg
from psycopg.rows import dict_row

DATABASE_URL = os.getenv('DATABASE_URL')
DB_HOST = os.getenv('DB_HOST', 'db')
DB_PORT = int(os.getenv('DB_PORT', '5432'))
DB_NAME = os.getenv('DB_NAME', 'hotel_booking')
DB_USER = os.getenv('DB_USER', 'hotel_user')
DB_PASSWORD = os.getenv('DB_PASSWORD', 'hotel_password')


def get_connection() -> psycopg.Connection:
    if DATABASE_URL:
        return psycopg.connect(DATABASE_URL, row_factory=dict_row)

    return psycopg.connect(
        host=DB_HOST,
        port=DB_PORT,
        dbname=DB_NAME,
        user=DB_USER,
        password=DB_PASSWORD,
        row_factory=dict_row,
    )


@contextmanager
def get_db_cursor():
    with get_connection() as conn:
        with conn.cursor() as cur:
            try:
                yield conn, cur
                conn.commit()
            except Exception:
                conn.rollback()
                raise


def init_db(retries: int = 20, delay: float = 1.0) -> None:
    last_error = None
    for _ in range(retries):
        try:
            with get_connection() as conn:
                with conn.cursor() as cur:
                    cur.execute('SELECT 1')
                    cur.fetchone()
            return
        except Exception as exc:  # pragma: no cover
            last_error = exc
            time.sleep(delay)

    raise RuntimeError('Could not connect to PostgreSQL') from last_error
