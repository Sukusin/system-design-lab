#include "token_store.hpp"

#include "security.hpp"

namespace hotel_booking_pg {

TokenStore::TokenStore(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context) {}

std::string TokenStore::Issue(std::string user_id, std::string login) {
    std::lock_guard lock(mutex_);
    CleanupLocked();

    auto token = GenerateToken();
    sessions_[token] = SessionData{
        .user_id = std::move(user_id),
        .login = std::move(login),
        .expires_at = std::chrono::system_clock::now() + std::chrono::minutes(60),
    };
    return token;
}

std::optional<SessionData> TokenStore::Resolve(std::string_view token) const {
    std::lock_guard lock(mutex_);
    CleanupLocked();

    const auto it = sessions_.find(std::string(token));
    if (it == sessions_.end()) return std::nullopt;
    return it->second;
}

void TokenStore::CleanupLocked() const {
    const auto now = std::chrono::system_clock::now();
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (it->second.expires_at <= now) {
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace hotel_booking_pg
