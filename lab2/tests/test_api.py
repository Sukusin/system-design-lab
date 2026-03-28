def test_register_user_success(client):
    response = client.post(
        '/auth/register',
        json={
            'login': 'kirill',
            'password': 'secret123',
            'first_name': 'Kirill',
            'last_name': 'Sokolov',
        },
    )
    assert response.status_code == 201
    data = response.json()
    assert data['login'] == 'kirill'
    assert data['first_name'] == 'Kirill'



def test_register_duplicate_login_returns_409(client):
    payload = {
        'login': 'kirill',
        'password': 'secret123',
        'first_name': 'Kirill',
        'last_name': 'Sokolov',
    }
    client.post('/auth/register', json=payload)
    response = client.post('/auth/register', json=payload)
    assert response.status_code == 409



def test_login_returns_jwt_token(client):
    client.post(
        '/auth/register',
        json={
            'login': 'kirill',
            'password': 'secret123',
            'first_name': 'Kirill',
            'last_name': 'Sokolov',
        },
    )
    response = client.post('/auth/login', json={'login': 'kirill', 'password': 'secret123'})
    assert response.status_code == 200
    assert 'access_token' in response.json()



def test_create_hotel_requires_authentication(client):
    response = client.post(
        '/hotels',
        json={
            'name': '5 Start Hotel',
            'city': 'Moscow',
            'address': 'Polevaya 5',
            'stars': 5,
            'rooms_total': 2,
        },
    )
    assert response.status_code == 401



def test_create_hotel_and_search_by_city(client, auth_headers):
    create_response = client.post(
        '/hotels',
        headers=auth_headers,
        json={
            'name': '5 Start Hotel',
            'city': 'Moscow',
            'address': 'Polevaya 5',
            'stars': 5,
            'rooms_total': 2,
        },
    )
    assert create_response.status_code == 201

    search_response = client.get('/hotels/search', params={'city': 'Moscow'})
    assert search_response.status_code == 200
    hotels = search_response.json()
    assert len(hotels) == 1
    assert hotels[0]['city'] == 'Moscow'



def test_create_booking_get_bookings_and_cancel(client, auth_headers):
    hotel_response = client.post(
        '/hotels',
        headers=auth_headers,
        json={
            'name': '5 Start Hotel',
            'city': 'Moscow',
            'address': 'Polevaya 5',
            'stars': 5,
            'rooms_total': 2,
        },
    )
    hotel_id = hotel_response.json()['id']

    booking_response = client.post(
        '/bookings',
        headers=auth_headers,
        json={
            'hotel_id': hotel_id,
            'check_in': '2026-04-10',
            'check_out': '2026-04-15',
            'guests': 2,
        },
    )
    assert booking_response.status_code == 201
    booking_id = booking_response.json()['id']

    bookings_response = client.get('/users/1/bookings', headers=auth_headers)
    assert bookings_response.status_code == 200
    assert len(bookings_response.json()) == 1

    cancel_response = client.patch(f'/bookings/{booking_id}/cancel', headers=auth_headers)
    assert cancel_response.status_code == 200
    assert cancel_response.json()['status'] == 'cancelled'



def test_booking_with_invalid_dates_returns_422(client, auth_headers):
    hotel_response = client.post(
        '/hotels',
        headers=auth_headers,
        json={
            'name': '5 Start Hotel',
            'city': 'Moscow',
            'address': 'Polevaya 5',
            'stars': 5,
            'rooms_total': 2,
        },
    )
    hotel_id = hotel_response.json()['id']

    response = client.post(
        '/bookings',
        headers=auth_headers,
        json={
            'hotel_id': hotel_id,
            'check_in': '2026-04-15',
            'check_out': '2026-04-10',
            'guests': 2,
        },
    )
    assert response.status_code == 422
