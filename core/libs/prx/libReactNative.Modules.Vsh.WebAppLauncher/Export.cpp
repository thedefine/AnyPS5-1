#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>

#include "HitLog.hpp"
#include "SceTypes.hpp"
#include "prx/libSceSystemService/SystemService.hpp"
#include "prx/libc/include/General.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

constexpr std::int32_t SHARE_ERROR_INVALID_PARAM = static_cast<std::int32_t>(0x81960002);

constexpr int ERROR_DIALOG_OK = 0;
constexpr int ERROR_DIALOG_ERROR_PARAM = static_cast<int>(0x80ED0003u);
constexpr int ERROR_DIALOG_ERROR_INVALID_STATE = static_cast<int>(0x80ED0005u);
constexpr std::size_t ERROR_DIALOG_PARAM_SIZE = 16;

std::mutex g_errorDialogLock;
bool g_errorDialogRunning = false;

bool Readable(const void* p, std::size_t n) {
#ifdef _WIN32
    if (p == nullptr) return false;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(p, &mbi, sizeof(mbi)) == 0 || mbi.State != MEM_COMMIT) return false;
    if ((mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) return false;
    const auto end = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    return reinterpret_cast<std::uintptr_t>(p) + n <= end;
#else
    (void)n;
    return p != nullptr;
#endif
}

int ConsoleLanguage() {
    const char* configured = std::getenv("ANYPS5_LANGUAGE");
    if (configured == nullptr || configured[0] == '\0') return SYSTEM_SERVICE_PARAM_LANG_ENGLISH_US;
    const std::string_view text(configured);
    int language = -1;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), language);
    if (error != std::errc{} || end != text.data() + text.size() || language < SYSTEM_SERVICE_PARAM_LANG_JAPANESE || language > SYSTEM_SERVICE_PARAM_LANG_UKRAINIAN) {
        throw std::runtime_error("SystemServiceParamGetInt: ANYPS5_LANGUAGE must be a console language number from " +
            std::to_string(SYSTEM_SERVICE_PARAM_LANG_JAPANESE) + " to " + std::to_string(SYSTEM_SERVICE_PARAM_LANG_UKRAINIAN) +
            ", not \"" + std::string(text) + "\"");
    }
    return language;
}

}

extern "C" {

int APS5_VABI ErrorDialogClose(void) {
    std::lock_guard g(g_errorDialogLock);
    if (!g_errorDialogRunning) return ERROR_DIALOG_ERROR_INVALID_STATE;
    g_errorDialogRunning = false;
    return ERROR_DIALOG_OK;
}

int APS5_VABI ErrorDialogOpen(const void* param) {
    std::lock_guard g(g_errorDialogLock);
    if (g_errorDialogRunning) return ERROR_DIALOG_ERROR_INVALID_STATE;
    if (!Readable(param, ERROR_DIALOG_PARAM_SIZE)) return ERROR_DIALOG_ERROR_PARAM;
    std::int32_t size = 0;
    std::memcpy(&size, param, sizeof(size));
    if (static_cast<std::size_t>(size) != ERROR_DIALOG_PARAM_SIZE) return ERROR_DIALOG_ERROR_PARAM;
    std::int32_t errorCode = 0;
    std::int32_t userId = 0;
    std::memcpy(&errorCode, static_cast<const std::uint8_t*>(param) + 4, sizeof(errorCode));
    std::memcpy(&userId, static_cast<const std::uint8_t*>(param) + 8, sizeof(userId));
    APS5_HIT("ERRORDIALOG", "open: error_code=0x%08X user_id=%d", errorCode, userId);
    g_errorDialogRunning = true;
    return ERROR_DIALOG_OK;
}

int APS5_VABI ShareGetCurrentStatus(uint32_t feature_flag, ShareCurrentStatus* status) {
    if (feature_flag == 0 || status == nullptr) {
        return SHARE_ERROR_INVALID_PARAM;
    }
    std::memset(status, 0, sizeof(*status));
    return 0;
}

int APS5_VABI ShareInitialize(size_t heap_size, int thread_priority, uint64_t affinity_mask) {
    (void)heap_size;
    (void)thread_priority;
    (void)affinity_mask;
    return 0;
}

int APS5_VABI ShareTerminate(void) {
    return 0;
}

int APS5_VABI SystemServiceParamGetInt(int param_id, int* value) {
    if (value == nullptr) {
        return SYSTEM_SERVICE_ERROR_PARAMETER;
    }
    switch (param_id) {
    case SYSTEM_SERVICE_PARAM_ID_LANG: *value = ConsoleLanguage(); break;
    case SYSTEM_SERVICE_PARAM_ID_DATE_FORMAT: *value = SYSTEM_SERVICE_PARAM_DATE_FORMAT_DDMMYYYY; break;
    case SYSTEM_SERVICE_PARAM_ID_TIME_FORMAT: *value = SYSTEM_SERVICE_PARAM_TIME_FORMAT_24HOUR; break;
    case SYSTEM_SERVICE_PARAM_ID_TIME_ZONE: *value = 0; break;
    case SYSTEM_SERVICE_PARAM_ID_SUMMERTIME: *value = 0; break;
    case SYSTEM_SERVICE_PARAM_ID_GAME_PARENTAL_LEVEL: *value = SYSTEM_SERVICE_PARAM_GAME_PARENTAL_OFF; break;
    case SYSTEM_SERVICE_PARAM_ID_ENTER_BUTTON_ASSIGN: *value = SYSTEM_SERVICE_PARAM_ENTER_BUTTON_CROSS; break;
    default: *value = 0; break;
    }
    return SYSTEM_SERVICE_OK;
}

int APS5_VABI SystemServiceParamGetString(int param_id, char* buf, size_t buf_size) {
    if (buf == nullptr || buf_size == 0) {
        return SYSTEM_SERVICE_ERROR_PARAMETER;
    }
    if (param_id != SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME) {
        NotImplemented_nid_no_patch("SystemServiceParamGetString: parameter other than the system name");
    }
    if (buf_size < SYSTEM_SERVICE_MAX_SYSTEM_NAME_LENGTH) {
        NotImplemented_nid_no_patch("SystemServiceParamGetString: buffer shorter than 65 bytes");
    }
    constexpr char SystemName[] = "PS5";
    std::memcpy(buf, SystemName, sizeof(SystemName));
    return SYSTEM_SERVICE_OK;
}

}
