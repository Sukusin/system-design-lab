import os
from datetime import datetime, timezone
from typing import Any

from bson import ObjectId
from pymongo import ASCENDING, DESCENDING, MongoClient
from pymongo.collection import Collection
from pymongo.database import Database

MONGO_URI = os.getenv('MONGO_URI', 'mongodb://root:root@mongo:27017/?authSource=admin')
MONGO_DB = os.getenv('MONGO_DB', 'hotel_booking_mongo')

_client: MongoClient | None = None


def get_client() -> MongoClient:
    global _client
    if _client is None:
        _client = MongoClient(MONGO_URI)
    return _client


def get_db() -> Database:
    return get_client()[MONGO_DB]


def users_collection() -> Collection:
    return get_db()['users']


def hotels_collection() -> Collection:
    return get_db()['hotels']


def bookings_collection() -> Collection:
    return get_db()['bookings']


def init_db() -> None:
    users_collection().create_index([('login', ASCENDING)], unique=True, name='ux_users_login')
    users_collection().create_index([('first_name', ASCENDING), ('last_name', ASCENDING)], name='ix_users_name')

    hotels_collection().create_index([('city', ASCENDING)], name='ix_hotels_city')
    hotels_collection().create_index([('created_by', ASCENDING)], name='ix_hotels_created_by')
    hotels_collection().create_index([('amenities', ASCENDING)], name='ix_hotels_amenities')

    bookings_collection().create_index([('user_id', ASCENDING), ('created_at', DESCENDING)], name='ix_bookings_user_created_at')
    bookings_collection().create_index([('hotel_id', ASCENDING), ('status', ASCENDING)], name='ix_bookings_hotel_status')
    bookings_collection().create_index(
        [('hotel_id', ASCENDING), ('status', ASCENDING), ('stay.check_in', ASCENDING), ('stay.check_out', ASCENDING)],
        name='ix_bookings_availability',
    )


def utc_now() -> datetime:
    return datetime.now(timezone.utc)


def object_id_from_string(value: str) -> ObjectId:
    if not ObjectId.is_valid(value):
        raise ValueError('Invalid ObjectId')
    return ObjectId(value)


def serialize_user(doc: dict[str, Any]) -> dict[str, Any]:
    return {
        'id': str(doc['_id']),
        'login': doc['login'],
        'first_name': doc['first_name'],
        'last_name': doc['last_name'],
        'created_at': doc['created_at'],
    }


def serialize_hotel(doc: dict[str, Any]) -> dict[str, Any]:
    return {
        'id': str(doc['_id']),
        'name': doc['name'],
        'city': doc['city'],
        'address': doc['address'],
        'stars': doc['stars'],
        'rooms_total': doc['rooms_total'],
        'amenities': doc.get('amenities', []),
        'created_by': str(doc['created_by']),
        'created_at': doc['created_at'],
    }


def serialize_booking(doc: dict[str, Any]) -> dict[str, Any]:
    stay = doc['stay']
    return {
        'id': str(doc['_id']),
        'user_id': str(doc['user_id']),
        'hotel_id': str(doc['hotel_id']),
        'check_in': stay['check_in'].date(),
        'check_out': stay['check_out'].date(),
        'guests': stay['guests'],
        'status': doc['status'],
        'hotel_snapshot': doc['hotel_snapshot'],
        'created_at': doc['created_at'],
        'cancelled_at': doc.get('cancelled_at'),
    }
