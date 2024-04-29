#include "D3d10App.h"
#include "nemulator.h"
#include "benchmark.h"
#include <memory>

std::unique_ptr<D3d10App> app;

struct cpu_info
{
    uint32_t is_intel;
    uint32_t is_amd;
    uint32_t is_early_zen;
    uint32_t has_bmi2;
} g_cpu_info;

cpu_info get_cpu_info()
{
    //EAX, EBX, ECX, EDX
    int cpu_info[4];
    struct cpu_info ci = {0};
    __cpuidex(cpu_info, 0, 0);
    int x = 1;
    char manufacturer_id[13];
    *(uint32_t *)&manufacturer_id[0] = cpu_info[1];
    *(uint32_t *)&manufacturer_id[4] = cpu_info[3];
    *(uint32_t *)&manufacturer_id[8] = cpu_info[2];
    manufacturer_id[12] = 0;
    if (strcmp(manufacturer_id, "GenuineIntel") == 0) {
        ci.is_intel = 1;
    }
    else if (strcmp(manufacturer_id, "AuthenticAMD") == 0) {
        ci.is_amd = 1;
    }
    __cpuidex(cpu_info, 1, 0);
    uint32_t family_id = (cpu_info[0] >> 8) & 0xF;
    uint32_t ext_family_id = (cpu_info[0] >> 20) & 0xFF;
    uint32_t proc_family_id = family_id;
    if (proc_family_id == 0xF) {
        proc_family_id += ext_family_id;
    }

    if (ci.is_amd && proc_family_id == 0x17) {
        ci.is_early_zen = 1;
    }
    __cpuidex(cpu_info, 7, 0);
    if (cpu_info[1] & 0x100) {
        ci.has_bmi2 = 1;
    }
    return ci;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdPline, int nShowCmd)
{
    g_cpu_info = get_cpu_info();
	app = std::make_unique<D3d10App>(hInstance);
	app->Init((char*)"nemulator.ini", new c_nemulator(), NULL);
	app->SetCaption("nemulator 4.5");
	int retval = app->Run();
	return retval;
}

void strip_extension(char *path)
{
	char *p = path + strlen(path) - 1;
	do
	{
		*p = 0;
	} while (*--p != '.');
	*p = 0;
}
