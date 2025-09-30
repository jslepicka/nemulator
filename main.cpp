#include "Windows.h"
#include <memory>
#include <string>

import D3d10App;
import nemulator;

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#ifdef PREVIEW_SHA
std::string app_title = "nemulator " STRINGIFY(PREVIEW_SHA);
#else
std::string app_title = "nemulator 5.0";
#endif

struct s_cpu_info
{
    uint32_t is_intel;
    uint32_t is_amd;
    uint32_t is_early_zen;
    uint32_t has_bmi2;
} g_cpu_info;

s_cpu_info get_cpu_info()
{
    union {
        struct
        {
            uint32_t EAX;
            uint32_t EBX;
            uint32_t ECX;
            uint32_t EDX;
        } reg;
        int i[4];
    } cpu_info = {0};
        
    s_cpu_info ci = {0};
    __cpuidex(cpu_info.i, 0, 0);
    int x = 1;
    char manufacturer_id[13];
    *(uint32_t *)&manufacturer_id[0] = cpu_info.reg.EBX;
    *(uint32_t *)&manufacturer_id[4] = cpu_info.reg.EDX;
    *(uint32_t *)&manufacturer_id[8] = cpu_info.reg.ECX;
    manufacturer_id[12] = 0;
    if (strcmp(manufacturer_id, "GenuineIntel") == 0) {
        ci.is_intel = 1;
    }
    else if (strcmp(manufacturer_id, "AuthenticAMD") == 0) {
        ci.is_amd = 1;
    }
    __cpuidex(cpu_info.i, 1, 0);
    uint32_t family_id = (cpu_info.reg.EAX >> 8) & 0xF;
    uint32_t ext_family_id = (cpu_info.reg.EAX >> 20) & 0xFF;
    uint32_t proc_family_id = family_id;
    if (proc_family_id == 0xF) {
        proc_family_id += ext_family_id;
    }

    if (ci.is_amd && proc_family_id == 0x17) {
        ci.is_early_zen = 1;
    }
    __cpuidex(cpu_info.i, 7, 0);
    if (cpu_info.reg.EBX & 0x100) {
        ci.has_bmi2 = 1;
    }
    return ci;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdPline, int nShowCmd)
{
    g_cpu_info = get_cpu_info();
    std::unique_ptr<D3d10App> app = std::make_unique<D3d10App>(hInstance);
    app->Init((char*)"nemulator.ini", new c_nemulator(), NULL);
    app->SetCaption(app_title);
    int retval = app->Run();
    return retval;
}