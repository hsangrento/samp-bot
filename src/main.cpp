#include <set>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef HAVE_STDINT_H
#define HAVE_STDINT_H
#endif

#include "plugincommon.h"
#include "amx/amx.h"

#include "urmem.hpp"

extern void *pAMXFunctions;
typedef void (*logprintf_t)(const char *fmt, ...);
static logprintf_t logprintf = nullptr;

static const char *kPluginName    = "sampbot";
static const char *kPluginVersion = "1.0";

static std::set<std::string> g_bots;

static std::shared_ptr<urmem::hook> g_addHook;

#ifdef _WIN32

static int __fastcall PB_OnAddPlayer(void *pool, void * /*edx*/, int playerid,
                                     char *name, int a3, int a4, int isNpc) {
    if (name && g_bots.find(std::string(name)) != g_bots.end())
        isNpc = 0;
    return g_addHook->call<urmem::calling_convention::thiscall, int>(
        pool, playerid, name, a3, a4, isNpc);
}

static bool BytesMatch(uintptr_t addr, const unsigned char *bytes, size_t n) {
    __try {
        const unsigned char *p = reinterpret_cast<const unsigned char *>(addr);
        for (size_t i = 0; i < n; ++i)
            if (p[i] != bytes[i])
                return false;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static bool VerifyServerBuild() {
    static const unsigned char sigQuery[]     = {0x81, 0x3B, 0x53, 0x41, 0x4D, 0x50};
    static const unsigned char sigAddPlayer[] = {0x6A, 0xFF, 0x64, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x68};
    return BytesMatch(0x0049E498, sigQuery, sizeof(sigQuery)) &&
           BytesMatch(0x004661C0, sigAddPlayer, sizeof(sigAddPlayer));
}

static void StartWorker() {
    if (!VerifyServerBuild()) {
        if (logprintf)
            logprintf("%s: unsupported samp-server.exe build, player integration disabled",
                      kPluginName);
        return;
    }
    g_addHook = urmem::hook::make(0x004661C0, &PB_OnAddPlayer);
    if (logprintf)
        logprintf("%s: player integration enabled", kPluginName);
}

static void StopWorker() {
    g_addHook.reset();
}

#else

static const char *PB_ADD_SIG =
    "\x55\x31\xD2\x89\xE5\x81\xEC\x98\x01\x00\x00\x8B\x45\x08\x89\x75\xF8"
    "\x0F\xB7\x75\x0C\x89\x5D\xF4\x89\x7D\xFC\x89\x85\x9C\xFE\xFF\xFF\x66"
    "\x81\xFE\x00\x00\x77\x12";
static const char *PB_ADD_MASK =
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx??xx";

static int PB_OnAddPlayer(void *a1, int playerid, char *name, int a4, int a5,
                          int isNpc) {
    if (name && g_bots.find(std::string(name)) != g_bots.end())
        isNpc = 0;
    return g_addHook->call<urmem::calling_convention::cdeclcall, int>(
        a1, playerid, name, a4, a5, isNpc);
}

static void StartWorker() {
    if (!logprintf)
        return;
    urmem::sig_scanner scanner;
    if (!scanner.init(reinterpret_cast<urmem::address_t>(logprintf)))
        return;
    urmem::address_t addr = 0;
    if (scanner.find(PB_ADD_SIG, PB_ADD_MASK, addr) && addr) {
        g_addHook = urmem::hook::make(addr, &PB_OnAddPlayer);
        logprintf("%s: player integration enabled", kPluginName);
    } else {
        logprintf("%s: unsupported samp03svr build, player integration disabled",
                  kPluginName);
    }
}

static void StopWorker() {
    g_addHook.reset();
}

#endif

static cell AMX_NATIVE_CALL n_PB_RegisterBot(AMX *amx, cell *params) {
    if (params[0] != static_cast<cell>(1 * sizeof(cell)))
        return 0;

    cell *addr = nullptr;
    if (amx_GetAddr(amx, params[1], &addr) != AMX_ERR_NONE)
        return 0;

    int len = 0;
    amx_StrLen(addr, &len);
    if (len <= 0)
        return 0;

    char *buf = static_cast<char *>(std::malloc(len + 1));
    if (!buf)
        return 0;
    std::memset(buf, 0, len + 1);

    if (amx_GetString(buf, addr, 0, len + 1) != AMX_ERR_NONE) {
        std::free(buf);
        return 0;
    }

    g_bots.insert(std::string(buf));
    std::free(buf);
    return 1;
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
    return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
    pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
    logprintf     = reinterpret_cast<logprintf_t>(ppData[PLUGIN_DATA_LOGPRINTF]);

    StartWorker();

    logprintf("%s plugin v%s by hsangrento loaded", kPluginName, kPluginVersion);
    return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
    StopWorker();
    g_bots.clear();
    if (logprintf)
        logprintf("%s plugin v%s by hsangrento unloaded", kPluginName, kPluginVersion);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
    static const AMX_NATIVE_INFO natives[] = {
        {"PB_RegisterBot", n_PB_RegisterBot},
        {nullptr, nullptr},
    };
    return amx_Register(amx, natives, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *) {
    return AMX_ERR_NONE;
}
