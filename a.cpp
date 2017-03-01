#include "a.h"
#include "nemulator.h"

D3d10App *app;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdPline, int nShowCmd)
{
//#if defined(DEBUG) | defined(_DEBUG)
//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF);
//#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
//#define new DEBUG_NEW
//#endif
	app = new D3d10App(hInstance);
	app->Init("nemulator.ini", new c_nemulator(), NULL);
	app->SetCaption("nemulator 4.2");
	int retval = app->Run();
	delete app;
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