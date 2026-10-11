#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

extern "C" {
int APS5_VABI RemotePlayGetConnectionStatus(int user_id, int* status);
int APS5_VABI ShareGetCurrentStatus(std::uint32_t feature_flag, void* status);
int APS5_VABI ShareInitialize(std::size_t heap_size, int thread_priority, std::uint64_t affinity_mask);
int APS5_VABI ShareTerminate(void);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

bool RejectsNullStatus(int userId) {
    try {
        RemotePlayGetConnectionStatus(userId, nullptr);
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

}

int main() {
    constexpr std::int32_t invalidParam = static_cast<std::int32_t>(0x81960002);

    int status = 7;
    Require(RemotePlayGetConnectionStatus(0, &status) == 0);
    Require(status == 0);

    status = 7;
    Require(RemotePlayGetConnectionStatus(-1, &status) == 0);
    Require(status == 0);

    Require(RejectsNullStatus(0));
    Require(RejectsNullStatus(-1));

    std::uint8_t raw[16];
    std::memset(raw, 0xAB, sizeof(raw));
    Require(ShareGetCurrentStatus(1, raw) == 0);
    for (std::size_t i = 0; i < sizeof(raw); ++i) {
        Require(raw[i] == 0);
    }

    std::memset(raw, 0xAB, sizeof(raw));
    Require(ShareGetCurrentStatus(0, raw) == invalidParam);
    Require(raw[0] == 0xAB);

    Require(ShareGetCurrentStatus(1, nullptr) == invalidParam);
    Require(ShareGetCurrentStatus(0, nullptr) == invalidParam);

    Require(ShareInitialize(0, 0, 0) == 0);
    Require(ShareInitialize(64 * 1024, 700, 0x3F) == 0);

    Require(ShareTerminate() == 0);
    Require(ShareTerminate() == 0);

    return 0;
}
