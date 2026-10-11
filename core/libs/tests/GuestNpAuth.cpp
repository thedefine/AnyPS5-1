#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdlib>

extern "C" {
int APS5_VABI sceNpAuthAbortRequest(int req_id);
int APS5_VABI sceNpAuthCreateAsyncRequest(const void* param);
int APS5_VABI sceNpAuthDeleteRequest(int req_id);
int APS5_VABI sceNpAuthGetAuthorizationCodeV3(int req_id, const void* param, void* auth_code, int* issuer_id);
int APS5_VABI sceNpAuthGetIdTokenV3(int req_id, const void* param, void* id_token);
int APS5_VABI sceNpAuthPollAsync(int req_id, int* result);
int APS5_VABI sceNpAuthWaitAsync(int req_id, int* result);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

}

int main() {
    constexpr int invalidArgument = static_cast<int>(0x80550003);
    constexpr int signedOut = static_cast<int>(0x80550006);
    constexpr int finished = 0;

    const int first = sceNpAuthCreateAsyncRequest(nullptr);
    const int second = sceNpAuthCreateAsyncRequest(nullptr);
    Require(first > 0);
    Require(second > first);

    int result = 7;
    Require(sceNpAuthPollAsync(first, &result) == finished);
    Require(result == signedOut);

    result = 7;
    Require(sceNpAuthWaitAsync(first, &result) == finished);
    Require(result == signedOut);
    Require(sceNpAuthWaitAsync(first, nullptr) == finished);

    unsigned char param[64]{};
    unsigned char token[64]{};
    Require(sceNpAuthGetIdTokenV3(first, param, token) == signedOut);
    for (std::size_t i = 0; i < sizeof(token); ++i) {
        Require(token[i] == 0);
    }

    Require(sceNpAuthGetIdTokenV3(first, nullptr, token) == invalidArgument);
    Require(sceNpAuthGetIdTokenV3(first, param, nullptr) == invalidArgument);
    Require(sceNpAuthGetIdTokenV3(first, nullptr, nullptr) == invalidArgument);

    int issuer = 7;
    Require(sceNpAuthGetAuthorizationCodeV3(first, param, token, &issuer) == signedOut);
    Require(sceNpAuthGetAuthorizationCodeV3(first, nullptr, token, &issuer) == invalidArgument);
    Require(sceNpAuthGetAuthorizationCodeV3(first, param, nullptr, &issuer) == invalidArgument);

    Require(sceNpAuthAbortRequest(first) == 0);
    Require(sceNpAuthDeleteRequest(first) == 0);
    Require(sceNpAuthDeleteRequest(second) == 0);

    return 0;
}
