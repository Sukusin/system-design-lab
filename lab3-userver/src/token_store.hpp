#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <userver/components/component_base.hpp>

namespace hotel_booking_pg {

struct SessionData {
    std::string user_id;
    std::string login;
    std::chrono::system_clock::time_point expires_at;
};

class TokenStore final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "token-store";

    TokenStore(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    std::string Issue(std::string user_id, std::string login);
    std::optional<SessionData> Resolve(std::string_view token) const;

private:
    void CleanupLocked() const;

    mutable std::mutex mutex_;
    mutable std::unordered_map<std::string, SessionData> sessions_;
};

}  // namespace hotel_booking_pg
