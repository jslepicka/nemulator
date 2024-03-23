#include "D3d10App.h"
#include "nemulator.h"
#include "benchmark.h"
#include <memory>

std::unique_ptr<D3d10App> app;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdPline, int nShowCmd)
{
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