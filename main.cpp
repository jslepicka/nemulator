#include "D3d10App.h"
#include "nemulator.h"
#include "benchmark.h""
#include <memory>

std::unique_ptr<D3d10App> app;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdPline, int nShowCmd)
{
	app = std::make_unique<D3d10App>(hInstance);
    auto nemulator = std::make_unique<c_nemulator>();
	app->Init((char*)"nemulator.ini", nemulator.get(), NULL);
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