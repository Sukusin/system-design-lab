$ErrorActionPreference = "Continue"
$BaseUrl = "http://localhost:8006"

Write-Host "1. Root endpoint"
Invoke-RestMethod "$BaseUrl/"

$random = Get-Random -Minimum 1000 -Maximum 9999
$login = "hw06_user_$random"

Write-Host "2. Register user: $login"
$registerBody = @{
  login = $login
  password = "secret123"
  first_name = "Kirill"
  last_name = "Sokolov"
} | ConvertTo-Json
$user = Invoke-RestMethod -Uri "$BaseUrl/auth/register" -Method Post -ContentType "application/json" -Body $registerBody
$user

Write-Host "3. Login"
$loginBody = @{
  login = $login
  password = "secret123"
} | ConvertTo-Json
$loginResponse = Invoke-RestMethod -Uri "$BaseUrl/auth/login" -Method Post -ContentType "application/json" -Body $loginBody
$loginResponse
$headers = @{ Authorization = "Bearer $($loginResponse.access_token)" }

Write-Host "4. Search/list endpoints"
Invoke-RestMethod "$BaseUrl/users/by-login/$login"
Invoke-RestMethod "$BaseUrl/users/search?mask=Kir"
Invoke-RestMethod "$BaseUrl/hotels" | Select-Object -First 2
Invoke-RestMethod "$BaseUrl/hotels/search?city=Moscow"

Write-Host "5. Create hotel -> HotelCreated event"
$hotelBody = @{
  name = "Event Hotel"
  city = "Moscow"
  address = "Event Street 10"
  stars = 5
  rooms_total = 30
} | ConvertTo-Json
$hotel = Invoke-RestMethod -Uri "$BaseUrl/hotels" -Method Post -ContentType "application/json" -Headers $headers -Body $hotelBody
$hotel

Write-Host "6. Create booking -> BookingCreated event"
$bookingBody = @{
  hotel_id = $hotel.id
  check_in = "2026-09-10"
  check_out = "2026-09-15"
  guests = 2
} | ConvertTo-Json
$booking = Invoke-RestMethod -Uri "$BaseUrl/bookings" -Method Post -ContentType "application/json" -Headers $headers -Body $bookingBody
$booking

Write-Host "7. User bookings read endpoint"
Invoke-RestMethod -Uri "$BaseUrl/users/$($user.id)/bookings" -Headers $headers

Write-Host "8. Cancel booking -> BookingCancelled event"
Invoke-RestMethod -Uri "$BaseUrl/bookings/$($booking.id)/cancel" -Method Patch -Headers $headers

Write-Host "9. Give consumer time to process RabbitMQ messages"
Start-Sleep -Seconds 3

Write-Host "10. Event log from PostgreSQL"
docker exec -i hotel-booking-hw06-db psql -U hotel_user -d hotel_booking -c "SELECT event_type, aggregate_type, aggregate_id, processed_at FROM event_log ORDER BY processed_at DESC LIMIT 10;"

Write-Host "11. CQRS read model from PostgreSQL"
docker exec -i hotel-booking-hw06-db psql -U hotel_user -d hotel_booking -c "SELECT booking_id, user_id, hotel_id, status, updated_at FROM booking_read_model ORDER BY updated_at DESC LIMIT 10;"

Write-Host "12. Consumer logs"
docker logs --tail 20 hotel-booking-hw06-consumer-userver
