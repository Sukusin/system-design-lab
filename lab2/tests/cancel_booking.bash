TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxIiwibG9naW4iOiJraXJpbGxfc29rb2xvdiIsImV4cCI6MTc3NDcyMzczMX0.BQAuZlE668xKZ3f6fxgSoTpRnNiMD5Sx-mHK62iDmvo"

curl -X PATCH \
  -H "Authorization: Bearer $TOKEN" \
  "http://127.0.0.1:8000/bookings/1/cancel"