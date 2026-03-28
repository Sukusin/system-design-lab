from datetime import datetime, timezone
from typing import List

from fastapi import Depends, FastAPI, HTTPException, Path, Query, status
from fastapi.responses import JSONResponse

from app.auth import auth_middleware, create_access_token, get_current_user, hash_password, verify_password
from app.database import get_db_cursor, init_db
from app.schemas import (
    BookingCreateRequest,
    BookingResponse,
    ErrorResponse,
    HotelCreateRequest,
    HotelResponse,
    MessageResponse,
    TokenResponse,
    UserLoginRequest,
    UserRegisterRequest,
    UserResponse,
)

app = FastAPI(
    title='Hotel Booking REST API',
    version='1.0.0',
    description='REST API сервис для бронирования отелей. Реализованы пользователи, отели, бронирования и JWT-аутентификация.',
)
app.middleware('http')(auth_middleware)


@app.on_event('startup')
def on_startup() -> None:
    init_db()


@app.post(
    '/auth/register',
    response_model=UserResponse,
    status_code=status.HTTP_201_CREATED,
    tags=['Authentication'],
    responses={409: {'model': ErrorResponse, 'description': 'Login already exists'}},
    summary='Создание нового пользователя',
)
def register_user(payload: UserRegisterRequest):
    now = datetime.now(timezone.utc).isoformat()
    with get_db_cursor() as (_, cur):
        cur.execute('SELECT id FROM users WHERE login = ?', (payload.login,))
        if cur.fetchone():
            raise HTTPException(status_code=409, detail='User with this login already exists')

        cur.execute(
            '''
            INSERT INTO users (login, password_hash, first_name, last_name, created_at)
            VALUES (?, ?, ?, ?, ?)
            ''',
            (payload.login, hash_password(payload.password), payload.first_name, payload.last_name, now),
        )
        user_id = cur.lastrowid
        cur.execute(
            'SELECT id, login, first_name, last_name, created_at FROM users WHERE id = ?',
            (user_id,),
        )
        row = cur.fetchone()
    return dict(row)


@app.post(
    '/auth/login',
    response_model=TokenResponse,
    tags=['Authentication'],
    responses={401: {'model': ErrorResponse, 'description': 'Invalid credentials'}},
    summary='Логин пользователя и получение JWT токена',
)
def login_user(payload: UserLoginRequest):
    with get_db_cursor() as (_, cur):
        cur.execute('SELECT * FROM users WHERE login = ?', (payload.login,))
        row = cur.fetchone()
        if not row or not verify_password(payload.password, row['password_hash']):
            raise HTTPException(status_code=401, detail='Invalid login or password')

    token = create_access_token(user_id=row['id'], login=row['login'])
    return {'access_token': token, 'token_type': 'bearer'}


@app.get(
    '/users/by-login/{login}',
    response_model=UserResponse,
    tags=['Users'],
    responses={404: {'model': ErrorResponse, 'description': 'User not found'}},
    summary='Поиск пользователя по логину',
)
def get_user_by_login(login: str = Path(..., min_length=3, max_length=50, examples=['kirill_sokolov'])):
    with get_db_cursor() as (_, cur):
        cur.execute(
            'SELECT id, login, first_name, last_name, created_at FROM users WHERE login = ?',
            (login,),
        )
        row = cur.fetchone()
        if not row:
            raise HTTPException(status_code=404, detail='User not found')
    return dict(row)


@app.get(
    '/users/search',
    response_model=List[UserResponse],
    tags=['Users'],
    summary='Поиск пользователя по маске имени и фамилии',
)
def search_users(mask: str = Query(..., min_length=1, example='Kir')):
    like_value = f'%{mask}%'
    with get_db_cursor() as (_, cur):
        cur.execute(
            '''
            SELECT id, login, first_name, last_name, created_at
            FROM users
            WHERE first_name LIKE ? OR last_name LIKE ?
            ORDER BY id ASC
            ''',
            (like_value, like_value),
        )
        rows = cur.fetchall()
    return [dict(row) for row in rows]


@app.post(
    '/hotels',
    response_model=HotelResponse,
    status_code=status.HTTP_201_CREATED,
    tags=['Hotels'],
    summary='Создание отеля',
    responses={401: {'model': ErrorResponse, 'description': 'Authentication required'}},
)
def create_hotel(payload: HotelCreateRequest, current_user=Depends(get_current_user)):
    now = datetime.now(timezone.utc).isoformat()
    with get_db_cursor() as (_, cur):
        cur.execute(
            '''
            INSERT INTO hotels (name, city, address, stars, rooms_total, created_by, created_at)
            VALUES (?, ?, ?, ?, ?, ?, ?)
            ''',
            (
                payload.name,
                payload.city,
                payload.address,
                payload.stars,
                payload.rooms_total,
                current_user['id'],
                now,
            ),
        )
        hotel_id = cur.lastrowid
        cur.execute('SELECT * FROM hotels WHERE id = ?', (hotel_id,))
        row = cur.fetchone()
    return dict(row)


@app.get(
    '/hotels',
    response_model=List[HotelResponse],
    tags=['Hotels'],
    summary='Получение списка отелей',
)
def list_hotels():
    with get_db_cursor() as (_, cur):
        cur.execute('SELECT * FROM hotels ORDER BY id ASC')
        rows = cur.fetchall()
    return [dict(row) for row in rows]


@app.get(
    '/hotels/search',
    response_model=List[HotelResponse],
    tags=['Hotels'],
    summary='Поиск отелей по городу',
)
def search_hotels_by_city(city: str = Query(..., min_length=1, example='Moscow')):
    with get_db_cursor() as (_, cur):
        cur.execute('SELECT * FROM hotels WHERE city LIKE ? ORDER BY id ASC', (f'%{city}%',))
        rows = cur.fetchall()
    return [dict(row) for row in rows]


@app.post(
    '/bookings',
    response_model=BookingResponse,
    status_code=status.HTTP_201_CREATED,
    tags=['Bookings'],
    summary='Создание бронирования',
    responses={
        401: {'model': ErrorResponse, 'description': 'Authentication required'},
        404: {'model': ErrorResponse, 'description': 'Hotel not found'},
        409: {'model': ErrorResponse, 'description': 'No available rooms for these dates'},
    },
)
def create_booking(payload: BookingCreateRequest, current_user=Depends(get_current_user)):
    now = datetime.now(timezone.utc).isoformat()
    with get_db_cursor() as (_, cur):
        cur.execute('SELECT * FROM hotels WHERE id = ?', (payload.hotel_id,))
        hotel = cur.fetchone()
        if not hotel:
            raise HTTPException(status_code=404, detail='Hotel not found')

        cur.execute(
            '''
            SELECT COUNT(*) AS active_bookings
            FROM bookings
            WHERE hotel_id = ?
              AND status = 'active'
              AND NOT (? <= check_in OR ? >= check_out)
            ''',
            (payload.hotel_id, payload.check_out.isoformat(), payload.check_in.isoformat()),
        )
        active_bookings = cur.fetchone()['active_bookings']

        if active_bookings >= hotel['rooms_total']:
            raise HTTPException(status_code=409, detail='No available rooms for selected dates')

        cur.execute(
            '''
            INSERT INTO bookings (user_id, hotel_id, check_in, check_out, guests, status, created_at, cancelled_at)
            VALUES (?, ?, ?, ?, ?, 'active', ?, NULL)
            ''',
            (
                current_user['id'],
                payload.hotel_id,
                payload.check_in.isoformat(),
                payload.check_out.isoformat(),
                payload.guests,
                now,
            ),
        )
        booking_id = cur.lastrowid
        cur.execute('SELECT * FROM bookings WHERE id = ?', (booking_id,))
        row = cur.fetchone()
    return dict(row)


@app.get(
    '/users/{user_id}/bookings',
    response_model=List[BookingResponse],
    tags=['Bookings'],
    summary='Получение бронирований пользователя',
    responses={401: {'model': ErrorResponse}, 403: {'model': ErrorResponse}},
)
def get_user_bookings(user_id: int, current_user=Depends(get_current_user)):
    if current_user['id'] != user_id:
        raise HTTPException(status_code=403, detail='Access denied: you can view only your own bookings')

    with get_db_cursor() as (_, cur):
        cur.execute('SELECT * FROM bookings WHERE user_id = ? ORDER BY id ASC', (user_id,))
        rows = cur.fetchall()
    return [dict(row) for row in rows]


@app.patch(
    '/bookings/{booking_id}/cancel',
    response_model=BookingResponse,
    tags=['Bookings'],
    summary='Отмена бронирования',
    responses={401: {'model': ErrorResponse}, 403: {'model': ErrorResponse}, 404: {'model': ErrorResponse}},
)
def cancel_booking(booking_id: int, current_user=Depends(get_current_user)):
    cancelled_at = datetime.now(timezone.utc).isoformat()
    with get_db_cursor() as (_, cur):
        cur.execute('SELECT * FROM bookings WHERE id = ?', (booking_id,))
        booking = cur.fetchone()
        if not booking:
            raise HTTPException(status_code=404, detail='Booking not found')
        if booking['user_id'] != current_user['id']:
            raise HTTPException(status_code=403, detail='Access denied: you can cancel only your own booking')
        if booking['status'] == 'cancelled':
            raise HTTPException(status_code=409, detail='Booking already cancelled')

        cur.execute(
            '''
            UPDATE bookings
            SET status = 'cancelled', cancelled_at = ?
            WHERE id = ?
            ''',
            (cancelled_at, booking_id),
        )
        cur.execute('SELECT * FROM bookings WHERE id = ?', (booking_id,))
        updated_booking = cur.fetchone()
    return dict(updated_booking)


@app.get('/', tags=['Health'])
def root():
    return {'message': 'Hotel Booking REST API is running'}
