#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

extern "C" {
int APS5_VABI sceShareCaptureScreenshot(const void* param, std::int32_t* req_id);
int APS5_VABI sceShareCaptureVideoClip(const void* param, std::int32_t* req_id);
int APS5_VABI sceShareGetCurrentStatus(std::uint32_t feature_flag, void* status);
int APS5_VABI sceShareOpenMenuForContent(const void* content_id);
int APS5_VABI sceShareFeaturePermit(std::uint32_t feature_flags);
int APS5_VABI sceShareInitialize(std::size_t heap_size, int thread_priority, std::uint64_t affinity_mask);
int APS5_VABI sceShareTerminate(void);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

}

int main() {
    constexpr std::int32_t notSupported = static_cast<std::int32_t>(0x81960007);
    constexpr std::int32_t invalidParam = static_cast<std::int32_t>(0x81960002);
    constexpr std::int32_t requestIdInvalid = -1;

    std::uint8_t param[64]{};
    std::int32_t reqId = 7;

    using Capture = int (APS5_VABI *)(const void*, std::int32_t*);
    for (Capture capture : {sceShareCaptureScreenshot, sceShareCaptureVideoClip}) {
        reqId = 7;
        Require(capture(param, &reqId) == notSupported);
        Require(reqId == requestIdInvalid);

        reqId = 7;
        Require(capture(nullptr, &reqId) == notSupported);
        Require(reqId == requestIdInvalid);

        Require(capture(param, nullptr) == notSupported);
        Require(capture(nullptr, nullptr) == notSupported);
    }

    std::uint8_t status[16];
    std::memset(status, 0xAB, sizeof(status));
    Require(sceShareGetCurrentStatus(1, status) == 0);
    for (std::size_t i = 0; i < sizeof(status); ++i) {
        Require(status[i] == 0);
    }

    std::memset(status, 0xAB, sizeof(status));
    Require(sceShareGetCurrentStatus(0, status) == invalidParam);
    Require(status[0] == 0xAB);

    Require(sceShareGetCurrentStatus(1, nullptr) == invalidParam);
    Require(sceShareGetCurrentStatus(0, nullptr) == invalidParam);

    Require(sceShareOpenMenuForContent(param) == notSupported);
    Require(sceShareOpenMenuForContent(nullptr) == notSupported);

    Require(sceShareFeaturePermit(0) == 0);
    Require(sceShareInitialize(0, 0, 0) == 0);
    Require(sceShareTerminate() == 0);

    return 0;
}
