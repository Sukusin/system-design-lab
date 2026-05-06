$BaseUrl = "http://localhost:8002"
$Login = "hw05_user_" + (Get-Random -Minimum 1000 -Maximum 9999)

Write-Host "1. Root endpoint"
curl.exe "$BaseUrl/"

Write-Host "`n2. Register user: $Login"
$registerBody = @{
  login = $Login
  password = "secret123"
  first_name = "Kirill"
  last_name = "Sokolov"
} | ConvertTo-Json

$user = Invoke-RestMethod -Uri "$BaseUrl/auth/register" -Method Post -ContentType "application/json" -Body $registerBody
$user

Write-Host "`n3. Login"
$loginBody = @{
  login = $Login
  password = "secret123"
} | ConvertTo-Json

$loginResponse = Invoke-RestMethod -Uri "$BaseUrl/auth/login" -Method Post -ContentType "application/json" -Body $loginBody
$token = $loginResponse.access_token
$headers = @{ Authorization = "Bearer $token" }
$loginResponse

Write-Host "`n4. Hotels cache MISS"
curl.exe -i "$BaseUrl/hotels"

Write-Host "`n5. Hotels cache HIT"
curl.exe -i "$BaseUrl/hotels"

Write-Host "`n6. City search cache"
curl.exe -i "$BaseUrl/hotels/search?city=Moscow"
curl.exe -i "$BaseUrl/hotels/search?city=Moscow"

Write-Host "`n7. Create hotel and invalidate hotels cache"
$hotelBody = @{
  name = "5 Star Hotel"
  city = "Moscow"
  address = "Polevaya 5"
  stars = 5
  rooms_total = 25
} | ConvertTo-Json

$hotel = Invoke-RestMethod -Uri "$BaseUrl/hotels" -Method Post -ContentType "application/json" -Headers $headers -Body $hotelBody
$hotel
curl.exe -i "$BaseUrl/hotels"

Write-Host "`n8. Create booking"
$bookingBody = @{
  hotel_id = [int64]$hotel.id
  check_in = "2026-06-10"
  check_out = "2026-06-15"
  guests = 2
} | ConvertTo-Json

$booking = Invoke-RestMethod -Uri "$BaseUrl/bookings" -Method Post -ContentType "application/json" -Headers $headers -Body $bookingBody
$booking

Write-Host "`n9. User bookings cache MISS then HIT"
$userId = $booking.user_id
curl.exe -i "$BaseUrl/users/$userId/bookings" -H "Authorization: Bearer $token"
curl.exe -i "$BaseUrl/users/$userId/bookings" -H "Authorization: Bearer $token"

Write-Host "`n10. Cancel booking and invalidate user bookings cache"
Invoke-RestMethod -Uri "$BaseUrl/bookings/$($booking.id)/cancel" -Method Patch -Headers $headers
curl.exe -i "$BaseUrl/users/$userId/bookings" -H "Authorization: Bearer $token"

Write-Host "`n11. Rate limit login example"
$wrongLoginBody = @{
  login = $Login
  password = "wrong_password"
} | ConvertTo-Json -Compress

1..7 | ForEach-Object {
  Write-Host "Attempt $_"
  try {
    $response = Invoke-WebRequest -Uri "$BaseUrl/auth/login" -Method Post -ContentType "application/json" -Body $wrongLoginBody
    Write-Host "HTTP $($response.StatusCode)"
    Write-Host $response.Content
  } catch {
    $status = $_.Exception.Response.StatusCode.value__
    Write-Host "HTTP $status"
    if ($_.Exception.Response.Headers["X-RateLimit-Limit"]) {
      Write-Host "X-RateLimit-Limit: $($_.Exception.Response.Headers["X-RateLimit-Limit"])"
      Write-Host "X-RateLimit-Remaining: $($_.Exception.Response.Headers["X-RateLimit-Remaining"])"
      Write-Host "X-RateLimit-Reset: $($_.Exception.Response.Headers["X-RateLimit-Reset"])"
      Write-Host "Retry-After: $($_.Exception.Response.Headers["Retry-After"])"
    }
    $stream = $_.Exception.Response.GetResponseStream()
    $reader = New-Object System.IO.StreamReader($stream)
    Write-Host $reader.ReadToEnd()
  }
}
