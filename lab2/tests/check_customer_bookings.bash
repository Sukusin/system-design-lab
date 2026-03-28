TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxIiwibG9naW4iOiJraXJpbGxfc29rb2xvdiIsImV4cCI6MTc3NDcyMzczMX0.BQAuZlE668xKZ3f6fxgSoTpRnNiMD5Sx-mHK62iDmvo"

curl -H "Authorization: Bearer $TOKEN" \
  "http://127.0.0.1:8000/users/1/bookings"