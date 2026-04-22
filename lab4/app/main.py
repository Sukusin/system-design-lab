import re
from datetime import datetime, time, timezone
from typing import List

from fastapi import Depends, FastAPI, HTTPException, Path, Query, status
from pymongo.errors import DuplicateKeyError

from app.auth import auth_middleware, create_access_token, get_current_user, hash_password, verify_password
from app.database import (
    bookings_collection,
    hotels_collection,
    init_db,
    object_id_from_string,
    serialize_booking,
    serialize_hotel,
    serialize_user,
    utc_now,
    users_collection,
)
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
    title='Hotel Booking REST API (MongoDB)',
    version='1.0.0',
    description='REST API сервис для бронирования отелей на MongoDB. Реализованы пользователи, отели, бронирования и JWT-аутентификация.',
)
app.middleware('http')(auth_middleware)


@app.on_event('startup')
def on_startup() -> None:
    init_db()


def date_to_utc_datetime(value):
    return datetime.combine(value, time.min, tzinfo=timezone.utc)


@app.post(
    '/auth/register',
    response_model=UserResponse,
    status_code=status.HTTP_201_CREATED,
    tags=['Authentication'],
    responses={409: {'model': ErrorResponse, 'description': 'Login already exists'}},
    summary='Создание нового пользователя',
)
def register_user(payload: UserRegisterRequest):
    doc = {
        'login': payload.login,
        'password_hash': hash_password(payload.password),
        'first_name': payload.first_name,
        'last_name': payload.last_name,
        'created_at': utc_now(),
    }
    try:
        result = users_collection().insert_one(doc)
    except DuplicateKeyError:
        raise HTTPException(status_code=409, detail='User with this login already exists')

    row = users_collection().find_one({'_id': result.inserted_id})
    return serialize_user(row)


@app.post(
    '/auth/login',
    response_model=TokenResponse,
    tags=['Authentication'],
    responses={401: {'model': ErrorResponse, 'description': 'Invalid credentials'}},
    summary='Логин пользователя и получение JWT токена',
)
def login_user(payload: UserLoginRequest):
    row = users_collection().find_one({'login': payload.login})
    if not row or not verify_password(payload.password, row['password_hash']):
        raise HTTPException(status_code=401, detail='Invalid login or password')

    token = create_access_token(user_id=str(row['_id']), login=row['login'])
    return {'access_token': token, 'token_type': 'bearer'}


@app.get(
    '/users/by-login/{login}',
    response_model=UserResponse,
    tags=['Users'],
    responses={404: {'model': ErrorResponse, 'description': 'User not found'}},
    summary='Поиск пользователя по логину',
)
def get_user_by_login(login: str = Path(..., min_length=3, max_length=50)):
    row = users_collection().find_one({'login': login})
    if not row:
        raise HTTPException(status_code=404, detail='User not found')
    return serialize_user(row)


@app.get(
    '/users/search',
    response_model=List[UserResponse],
    tags=['Users'],
    summary='Поиск пользователя по маске имени и фамилии',
)
def search_users(mask: str = Query(..., min_length=1, examples=['Kir'])):
    regex = {'$regex': re.escape(mask), '$options': 'i'}
    rows = users_collection().find(
        {'$or': [{'first_name': regex}, {'last_name': regex}]},
        sort=[('first_name', 1), ('last_name', 1), ('_id', 1)],
    )
    return [serialize_user(row) for row in rows]


@app.post(
    '/hotels',
    response_model=HotelResponse,
    status_code=status.HTTP_201_CREATED,
    tags=['Hotels'],
    summary='Создание отеля',
    responses={401: {'model': ErrorResponse, 'description': 'Authentication required'}},
)
def create_hotel(payload: HotelCreateRequest, current_user=Depends(get_current_user)):
    clean_amenities = sorted({item.strip() for item in payload.amenities if item.strip()}) or ['wifi']
    doc = {
        'name': payload.name,
        'city': payload.city,
        'address': payload.address,
        'stars': payload.stars,
        'rooms_total': payload.rooms_total,
        'amenities': clean_amenities,
        'created_by': current_user['_id'],
        'created_at': utc_now(),
    }
    result = hotels_collection().insert_one(doc)
    row = hotels_collection().find_one({'_id': result.inserted_id})
    return serialize_hotel(row)


@app.get(
    '/hotels',
    response_model=List[HotelResponse],
    tags=['Hotels'],
    summary='Получение списка отелей',
)
def list_hotels():
    rows = hotels_collection().find(sort=[('city', 1), ('name', 1), ('_id', 1)])
    return [serialize_hotel(row) for row in rows]


@app.get(
    '/hotels/search',
    response_model=List[HotelResponse],
    tags=['Hotels'],
    summary='Поиск отелей по городу',
)
def search_hotels_by_city(city: str = Query(..., min_length=1, examples=['Moscow'])):
    regex = {'$regex': re.escape(city), '$options': 'i'}
    rows = hotels_collection().find({'city': regex}, sort=[('name', 1), ('_id', 1)])
    return [serialize_hotel(row) for row in rows]


@app.post(
    '/bookings',
    response_model=BookingResponse,
    status_code=status.HTTP_201_CREATED,
    tags=['Bookings'],
    summary='Создание бронирования',
    responses={
        400: {'model': ErrorResponse, 'description': 'Invalid ObjectId'},
        401: {'model': ErrorResponse, 'description': 'Authentication required'},
        404: {'model': ErrorResponse, 'description': 'Hotel not found'},
        409: {'model': ErrorResponse, 'description': 'No available rooms for these dates'},
    },
)
def create_booking(payload: BookingCreateRequest, current_user=Depends(get_current_user)):
    try:
        hotel_id = object_id_from_string(payload.hotel_id)
    except ValueError:
        raise HTTPException(status_code=400, detail='Invalid hotel_id')

    hotel = hotels_collection().find_one({'_id': hotel_id})
    if not hotel:
        raise HTTPException(status_code=404, detail='Hotel not found')

    check_in_dt = date_to_utc_datetime(payload.check_in)
    check_out_dt = date_to_utc_datetime(payload.check_out)

    active_bookings = bookings_collection().count_documents(
        {
            'hotel_id': hotel_id,
            'status': 'active',
            'stay.check_in': {'$lt': check_out_dt},
            'stay.check_out': {'$gt': check_in_dt},
        }
    )
    if active_bookings >= hotel['rooms_total']:
        raise HTTPException(status_code=409, detail='No available rooms for selected dates')

    now = utc_now()
    doc = {
        'user_id': current_user['_id'],
        'hotel_id': hotel_id,
        'hotel_snapshot': {
            'name': hotel['name'],
            'city': hotel['city'],
            'address': hotel['address'],
            'stars': hotel['stars'],
        },
        'stay': {
            'check_in': check_in_dt,
            'check_out': check_out_dt,
            'guests': payload.guests,
        },
        'status': 'active',
        'created_at': now,
        'cancelled_at': None,
        'status_history': [
            {'status': 'active', 'changed_at': now, 'comment': 'booking created'},
        ],
    }
    result = bookings_collection().insert_one(doc)
    row = bookings_collection().find_one({'_id': result.inserted_id})
    return serialize_booking(row)


@app.get(
    '/users/{user_id}/bookings',
    response_model=List[BookingResponse],
    tags=['Bookings'],
    summary='Получение бронирований пользователя',
    responses={401: {'model': ErrorResponse}, 403: {'model': ErrorResponse}, 400: {'model': ErrorResponse}},
)
def get_user_bookings(user_id: str, current_user=Depends(get_current_user)):
    if str(current_user['_id']) != user_id:
        raise HTTPException(status_code=403, detail='Access denied: you can view only your own bookings')

    try:
        user_object_id = object_id_from_string(user_id)
    except ValueError:
        raise HTTPException(status_code=400, detail='Invalid user_id')

    rows = bookings_collection().find({'user_id': user_object_id}, sort=[('created_at', -1), ('_id', 1)])
    return [serialize_booking(row) for row in rows]


@app.patch(
    '/bookings/{booking_id}/cancel',
    response_model=BookingResponse,
    tags=['Bookings'],
    summary='Отмена бронирования',
    responses={401: {'model': ErrorResponse}, 403: {'model': ErrorResponse}, 404: {'model': ErrorResponse}, 400: {'model': ErrorResponse}},
)
def cancel_booking(booking_id: str, current_user=Depends(get_current_user)):
    try:
        booking_object_id = object_id_from_string(booking_id)
    except ValueError:
        raise HTTPException(status_code=400, detail='Invalid booking_id')

    booking = bookings_collection().find_one({'_id': booking_object_id})
    if not booking:
        raise HTTPException(status_code=404, detail='Booking not found')
    if booking['user_id'] != current_user['_id']:
        raise HTTPException(status_code=403, detail='Access denied: you can cancel only your own booking')
    if booking['status'] == 'cancelled':
        raise HTTPException(status_code=409, detail='Booking already cancelled')

    cancelled_at = utc_now()
    bookings_collection().update_one(
        {'_id': booking_object_id},
        {
            '$set': {'status': 'cancelled', 'cancelled_at': cancelled_at},
            '$push': {'status_history': {'status': 'cancelled', 'changed_at': cancelled_at, 'comment': 'cancelled by user'}},
        },
    )
    updated_booking = bookings_collection().find_one({'_id': booking_object_id})
    return serialize_booking(updated_booking)


@app.get('/', tags=['Health'])
def root():
    return {'message': 'Hotel Booking REST API with MongoDB is running'}
