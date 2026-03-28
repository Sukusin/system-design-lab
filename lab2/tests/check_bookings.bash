TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxIiwibG9naW4iOiJraXJpbGxfc29rb2xvdiIsImV4cCI6MTc3NDcyMzczMX0.BQAuZlE668xKZ3f6fxgSoTpRnNiMD5Sx-mHK62iDmvo"

curl -X POST "http://127.0.0.1:8000/bookings" \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{
    "hotel_id": 1,
    "check_in": "2026-04-10",
    "check_out": "2026-04-15",
    "guests": 2
  }'