TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxIiwibG9naW4iOiJraXJpbGxfc29rb2xvdiIsImV4cCI6MTc3NDcyMzczMX0.BQAuZlE668xKZ3f6fxgSoTpRnNiMD5Sx-mHK62iDmvo"

curl -X POST "http://127.0.0.1:8000/hotels" \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{
    "name": "5 Strt Hote",
    "city": "Moscow",
    "address": "Polevaya 5",
    "stars": 5,
    "rooms_total": 20
  }'