from datetime import date, datetime
from typing import Literal, Optional

from pydantic import BaseModel, ConfigDict, Field, field_validator


class ErrorResponse(BaseModel):
    detail: str = Field(..., examples=['Resource not found'])


class UserRegisterRequest(BaseModel):
    login: str = Field(..., min_length=3, max_length=50, examples=['kirill_sokolov'])
    password: str = Field(..., min_length=6, max_length=128, examples=['secret123'])
    first_name: str = Field(..., min_length=1, max_length=100, examples=['Kirill'])
    last_name: str = Field(..., min_length=1, max_length=100, examples=['Sokolov'])


class UserLoginRequest(BaseModel):
    login: str = Field(..., examples=['kirill_sokolov'])
    password: str = Field(..., examples=['secret123'])


class UserResponse(BaseModel):
    id: str
    login: str
    first_name: str
    last_name: str
    created_at: datetime

    model_config = ConfigDict(from_attributes=True)


class TokenResponse(BaseModel):
    access_token: str
    token_type: str = 'bearer'


class HotelCreateRequest(BaseModel):
    name: str = Field(..., min_length=2, max_length=200, examples=['5 Star Hotel'])
    city: str = Field(..., min_length=2, max_length=100, examples=['Moscow'])
    address: str = Field(..., min_length=5, max_length=255, examples=['Polevaya 5'])
    stars: int = Field(..., ge=1, le=5, examples=[4])
    rooms_total: int = Field(..., ge=1, le=10000, examples=[25])
    amenities: list[str] = Field(default_factory=lambda: ['wifi'])


class HotelResponse(BaseModel):
    id: str
    name: str
    city: str
    address: str
    stars: int
    rooms_total: int
    amenities: list[str]
    created_by: str
    created_at: datetime


class BookingCreateRequest(BaseModel):
    hotel_id: str = Field(..., examples=['65f000000000000000000101'])
    check_in: date = Field(..., examples=['2026-05-10'])
    check_out: date = Field(..., examples=['2026-05-15'])
    guests: int = Field(..., ge=1, le=10, examples=[2])

    @field_validator('check_out')
    @classmethod
    def validate_dates(cls, check_out: date, info):
        check_in = info.data.get('check_in')
        if check_in and check_out <= check_in:
            raise ValueError('check_out must be later than check_in')
        return check_out


class BookingSnapshot(BaseModel):
    name: str
    city: str
    address: str
    stars: int


class BookingResponse(BaseModel):
    id: str
    user_id: str
    hotel_id: str
    check_in: date
    check_out: date
    guests: int
    status: Literal['active', 'cancelled']
    hotel_snapshot: BookingSnapshot
    created_at: datetime
    cancelled_at: Optional[datetime] = None


class MessageResponse(BaseModel):
    message: str = Field(..., examples=['Booking cancelled successfully'])
