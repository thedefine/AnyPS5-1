#include "SceTypes.hpp"
#include "prx/libSceSystemService/SystemService.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

extern "C" {
int APS5_VABI ErrorDialogClose(void);
int APS5_VABI ErrorDialogOpen(const void* param);
int APS5_VABI ShareGetCurrentStatus(std::uint32_t feature_flag, void* status);
int APS5_VABI ShareInitialize(std::size_t heap_size, int thread_priority, std::uint64_t affinity_mask);
int APS5_VABI ShareTerminate(void);
int APS5_VABI SystemServiceParamGetInt(int param_id, int* value);
int APS5_VABI SystemServiceParamGetString(int param_id, char* buf, std::size_t buf_size);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

bool RejectsString(int paramId, char* buf, std::size_t size) {
    try {
        SystemServiceParamGetString(paramId, buf, size);
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

}

int main() {
    constexpr int dialogOk = 0;
    constexpr int dialogParam = static_cast<int>(0x80ED0003u);
    constexpr int dialogInvalidState = static_cast<int>(0x80ED0005u);
    constexpr std::int32_t shareInvalidParam = static_cast<std::int32_t>(0x81960002);

    std::uint8_t param[16]{};
    const std::int32_t goodSize = static_cast<std::int32_t>(sizeof(param));
    std::memcpy(param, &goodSize, sizeof(goodSize));
    const std::int32_t errorCode = static_cast<std::int32_t>(0x80990001u);
    std::memcpy(param + 4, &errorCode, sizeof(errorCode));

    Require(ErrorDialogClose() == dialogInvalidState);
    Require(ErrorDialogOpen(nullptr) == dialogParam);
    Require(ErrorDialogOpen(param) == dialogOk);
    Require(ErrorDialogOpen(param) == dialogInvalidState);
    Require(ErrorDialogClose() == dialogOk);
    Require(ErrorDialogClose() == dialogInvalidState);
    Require(ErrorDialogOpen(param) == dialogOk);
    Require(ErrorDialogClose() == dialogOk);

    const std::int32_t shortSize = goodSize - 1;
    std::memcpy(param, &shortSize, sizeof(shortSize));
    Require(ErrorDialogOpen(param) == dialogParam);
    std::memcpy(param, &goodSize, sizeof(goodSize));
    Require(ErrorDialogOpen(param) == dialogOk);
    Require(ErrorDialogClose() == dialogOk);

    std::uint8_t raw[16];
    std::memset(raw, 0xAB, sizeof(raw));
    Require(ShareGetCurrentStatus(1, raw) == 0);
    for (std::size_t i = 0; i < sizeof(raw); ++i) {
        Require(raw[i] == 0);
    }
    std::memset(raw, 0xAB, sizeof(raw));
    Require(ShareGetCurrentStatus(0, raw) == shareInvalidParam);
    Require(raw[0] == 0xAB);
    Require(ShareGetCurrentStatus(1, nullptr) == shareInvalidParam);
    Require(ShareInitialize(0, 0, 0) == 0);
    Require(ShareInitialize(64 * 1024, 700, 0x3F) == 0);
    Require(ShareTerminate() == 0);

    int value = -1;
    Require(SystemServiceParamGetInt(SYSTEM_SERVICE_PARAM_ID_DATE_FORMAT, &value) == SYSTEM_SERVICE_OK);
    Require(value == SYSTEM_SERVICE_PARAM_DATE_FORMAT_DDMMYYYY);

    value = -1;
    Require(SystemServiceParamGetInt(SYSTEM_SERVICE_PARAM_ID_TIME_FORMAT, &value) == SYSTEM_SERVICE_OK);
    Require(value == SYSTEM_SERVICE_PARAM_TIME_FORMAT_24HOUR);

    value = -1;
    Require(SystemServiceParamGetInt(SYSTEM_SERVICE_PARAM_ID_ENTER_BUTTON_ASSIGN, &value) == SYSTEM_SERVICE_OK);
    Require(value == SYSTEM_SERVICE_PARAM_ENTER_BUTTON_CROSS);

    value = -1;
    Require(SystemServiceParamGetInt(SYSTEM_SERVICE_PARAM_ID_TIME_ZONE, &value) == SYSTEM_SERVICE_OK);
    Require(value == 0);

    value = -1;
    Require(SystemServiceParamGetInt(999999, &value) == SYSTEM_SERVICE_OK);
    Require(value == 0);

    Require(SystemServiceParamGetInt(SYSTEM_SERVICE_PARAM_ID_LANG, nullptr) == SYSTEM_SERVICE_ERROR_PARAMETER);

    value = -1;
    Require(SystemServiceParamGetInt(SYSTEM_SERVICE_PARAM_ID_LANG, &value) == SYSTEM_SERVICE_OK);
    Require(value >= SYSTEM_SERVICE_PARAM_LANG_JAPANESE && value <= SYSTEM_SERVICE_PARAM_LANG_UKRAINIAN);

    char name[SYSTEM_SERVICE_MAX_SYSTEM_NAME_LENGTH]{};
    std::memset(name, 0xCD, sizeof(name));
    Require(SystemServiceParamGetString(SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME, name, sizeof(name)) == SYSTEM_SERVICE_OK);
    Require(std::strcmp(name, "PS5") == 0);

    Require(SystemServiceParamGetString(SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME, nullptr, sizeof(name)) == SYSTEM_SERVICE_ERROR_PARAMETER);
    Require(SystemServiceParamGetString(SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME, name, 0) == SYSTEM_SERVICE_ERROR_PARAMETER);

    Require(RejectsString(SYSTEM_SERVICE_PARAM_ID_LANG, name, sizeof(name)));
    Require(RejectsString(SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME, name, SYSTEM_SERVICE_MAX_SYSTEM_NAME_LENGTH - 1));

    return 0;
}
