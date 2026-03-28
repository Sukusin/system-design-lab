import os
import sys
from pathlib import Path

import pytest
from fastapi.testclient import TestClient

ROOT_DIR = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT_DIR))

TEST_DB = ROOT_DIR / 'test_hotel_booking.db'
os.environ['DB_PATH'] = str(TEST_DB)

from app.main import app  # noqa: E402
from app.database import init_db  # noqa: E402


@pytest.fixture(autouse=True)
def reset_db():
    if TEST_DB.exists():
        TEST_DB.unlink()
    init_db()
    yield
    if TEST_DB.exists():
        TEST_DB.unlink()


@pytest.fixture()
def client():
    with TestClient(app) as c:
        yield c


@pytest.fixture()
def auth_headers(client):
    register_payload = {
        'login': 'tester',
        'password': 'secret123',
        'first_name': 'Test',
        'last_name': 'User',
    }
    client.post('/auth/register', json=register_payload)
    login_resp = client.post('/auth/login', json={'login': 'tester', 'password': 'secret123'})
    token = login_resp.json()['access_token']
    return {'Authorization': f'Bearer {token}'}
