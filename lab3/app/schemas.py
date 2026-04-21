from datetime import date, datetime
from typing import Literal, Optional

from pydantic import BaseModel, ConfigDict, Field, field_validator


class ErrorResponse(BaseModel):
    detail: str = Field(..., example='Resource not found')


class UserRegisterRequest(BaseModel):
    login: str = Field(..., min_length=3, max_length=50, example='kirill_sokolov')
    password: str = Field(..., min_length=6, max_length=128, example='secret123')
    first_name: str = Field(..., min_length=1, max_length=100, example='Kirill')
    last_name: str = Field(..., min_length=1, max_length=100, example='Sokolov')


class UserLoginRequest(BaseModel):
    login: str = Field(..., example='kirill_sokolov')
    password: str = Field(..., example='secret123')


class UserResponse(BaseModel):
    id: int
    login: str
    first_name: str
    last_name: str
    created_at: datetime

    model_config = ConfigDict(from_attributes=True)


class TokenResponse(BaseModel):
    access_token: str
    token_type: str = 'bearer'


class HotelCreateRequest(BaseModel):
    name: str = Field(..., min_length=2, max_length=200, example='5 Star Hotel')
    city: str = Field(..., min_length=2, max_length=100, example='Moscow')
    address: str = Field(..., min_length=5, max_length=255, example='Polevaya 5')
    stars: int = Field(..., ge=1, le=5, example=5)
    rooms_total: int = Field(..., ge=1, le=10000, example=25)


class HotelResponse(BaseModel):
    id: int
    name: str
    city: str
    address: str
    stars: int
    rooms_total: int
    created_by: int
    created_at: datetime


class BookingCreateRequest(BaseModel):
    hotel_id: int = Field(..., example=1)
    check_in: date = Field(..., example='2026-04-10')
    check_out: date = Field(..., example='2026-04-15')
    guests: int = Field(..., ge=1, le=10, example=2)

    @field_validator('check_out')
    @classmethod
    def validate_dates(cls, check_out: date, info):
        check_in = info.data.get('check_in')
        if check_in and check_out <= check_in:
            raise ValueError('check_out must be later than check_in')
        return check_out


class BookingResponse(BaseModel):
    id: int
    user_id: int
    hotel_id: int
    check_in: date
    check_out: date
    guests: int
    status: Literal['active', 'cancelled']
    created_at: datetime
    cancelled_at: Optional[datetime] = None


class MessageResponse(BaseModel):
    message: str = Field(..., example='Booking cancelled successfully')
