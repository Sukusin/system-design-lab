import hashlib
import hmac
import os
import secrets
from datetime import datetime, timedelta, timezone
from typing import Optional

from fastapi import HTTPException, Request, status
from jose import JWTError, jwt

from app.database import get_db_cursor

SECRET_KEY = os.getenv('JWT_SECRET_KEY', 'super-secret-key-change-me')
ALGORITHM = 'HS256'
ACCESS_TOKEN_EXPIRE_MINUTES = 60


def hash_password(password: str) -> str:
    salt = secrets.token_hex(16)
    password_hash = hashlib.pbkdf2_hmac('sha256', password.encode(), salt.encode(), 100_000).hex()
    return f'{salt}${password_hash}'


def verify_password(plain_password: str, stored_hash: str) -> bool:
    try:
        salt, password_hash = stored_hash.split('$', 1)
    except ValueError:
        return False

    candidate_hash = hashlib.pbkdf2_hmac('sha256', plain_password.encode(), salt.encode(), 100_000).hex()
    return hmac.compare_digest(candidate_hash, password_hash)


def create_access_token(user_id: int, login: str) -> str:
    expire = datetime.now(timezone.utc) + timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
    payload = {'sub': str(user_id), 'login': login, 'exp': expire}
    return jwt.encode(payload, SECRET_KEY, algorithm=ALGORITHM)


def decode_token(token: str) -> Optional[dict]:
    try:
        return jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
    except JWTError:
        return None


async def auth_middleware(request: Request, call_next):
    request.state.user = None
    auth_header = request.headers.get('Authorization')

    if auth_header and auth_header.startswith('Bearer '):
        token = auth_header.split(' ', 1)[1]
        payload = decode_token(token)
        if payload:
            user_id = payload.get('sub')
            with get_db_cursor() as (_, cur):
                cur.execute(
                    '''
                    SELECT id, login, first_name, last_name, created_at
                    FROM users
                    WHERE id = %s
                    ''',
                    (user_id,),
                )
                row = cur.fetchone()
                if row:
                    request.state.user = row

    response = await call_next(request)
    return response


def get_current_user(request: Request):
    if not request.state.user:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail='Authentication required',
            headers={'WWW-Authenticate': 'Bearer'},
        )
    return request.state.user
