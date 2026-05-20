#pragma once

#include <string>
#include <string_view>

namespace hotel_booking_pg {

std::string HashPassword(std::string_view password);
bool VerifyPassword(std::string_view password, std::string_view stored_hash);
std::string GenerateToken();

}  // namespace hotel_booking_pg
