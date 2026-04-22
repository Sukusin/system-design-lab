#include "security.hpp"

#include <array>
#include <stdexcept>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace hotel_booking_mongo {
namespace {

std::string ToHex(const unsigned char* data, std::size_t size) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.resize(size * 2);
    for (std::size_t i = 0; i < size; ++i) {
        out[i * 2] = kHex[(data[i] >> 4) & 0x0F];
        out[i * 2 + 1] = kHex[data[i] & 0x0F];
    }
    return out;
}

std::string RandomHex(std::size_t size) {
    std::string bytes;
    bytes.resize(size);
    if (RAND_bytes(reinterpret_cast<unsigned char*>(bytes.data()), static_cast<int>(size)) != 1) {
        throw std::runtime_error("RAND_bytes failed");
    }
    return ToHex(reinterpret_cast<const unsigned char*>(bytes.data()), size);
}

std::string Derive(std::string_view password, std::string_view salt) {
    std::array<unsigned char, 32> hash{};
    if (PKCS5_PBKDF2_HMAC(
            password.data(),
            static_cast<int>(password.size()),
            reinterpret_cast<const unsigned char*>(salt.data()),
            static_cast<int>(salt.size()),
            100000,
            EVP_sha256(),
            static_cast<int>(hash.size()),
            hash.data()) != 1) {
        throw std::runtime_error("PKCS5_PBKDF2_HMAC failed");
    }
    return ToHex(hash.data(), hash.size());
}

}  // namespace

std::string HashPassword(std::string_view password) {
    const auto salt = RandomHex(16);
    return salt + "$" + Derive(password, salt);
}

bool VerifyPassword(std::string_view password, std::string_view stored_hash) {
    const auto pos = stored_hash.find('$');
    if (pos == std::string_view::npos) return false;

    const auto salt = stored_hash.substr(0, pos);
    const auto expected = stored_hash.substr(pos + 1);
    const auto actual = Derive(password, salt);

    if (actual.size() != expected.size()) return false;
    return CRYPTO_memcmp(actual.data(), expected.data(), actual.size()) == 0;
}

std::string GenerateToken() {
    return RandomHex(32);
}

}  // namespace hotel_booking_mongo
