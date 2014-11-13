#include "Game.h"
#include <crtdbg.h>
//#if defined(DEBUG) | defined(_DEBUG)
//#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
//#define new DEBUG_NEW
//#endif
extern int pal[64 * 8];
extern HANDLE g_start_event;

DWORD WINAPI Game::NesThread(LPVOID lpParam)
{
	Game *g = (Game*) lpParam;
	while (true)
	{
		WaitForMultipleObjects(2, g->h, FALSE, INFINITE);
		if (g->kill)
			break;
		if (g->nes->loaded)
			//g->nes->EmulateFrame();
			g->nes->emulate_frame();
		SetEvent(g->eventDone);
	}
	//delete g->nes;
	//g->nes = 0;
	g->kill = false;
	g->DestroyEvents();
	//CloseHandle(g->threadHandle);
	//g->threadHandle = 0;
	return 0;
}

std::string Game::get_filename()
{
	return filename;
}

Game::Game(std::string path, std::string filename, std::string sram_path)
{
	emulation_mode = c_nes::EMULATION_MODE_FAST;
	limit_sprites = false;
	this->path = path;
	this->filename = filename;
	this->sram_path = sram_path;
	ref = 0;
	nes = 0;
	running = false;
	kill = false;
	threadHandle = NULL;
	eventStart = NULL;
	eventDone = NULL;
	eventKill = NULL;
	mask_sides = false;
	favorite = false;
	submapper = 0;
}

Game::~Game(void)
{
	if (nes)
		delete nes;
}

void Game::OnLoad()
{
	//if (!nes)
	//{
	//	nes = new c_nes();
	//	nes->emulate_mode = emulation_mode;
	//	char fn[MAX_PATH];
	//	char p[MAX_PATH];
	//	strcpy_s(fn, MAX_PATH, filename.c_str());
	//	strcpy_s(p, MAX_PATH, path.c_str());
	//	nes->Load(p, fn);
	//	if (nes->loaded)
	//		InitThread();
	//}
	if (!nes->loaded)
	{
		nes->Load(/*p, fn*/);
		if (nes->loaded)
		{
			//nes->set_submapper(submapper);
			nes->set_sprite_limit(limit_sprites);
			InitThread();
		}
	}
}

void Game::OnActivate(bool load)
{
	if (ref == 0)
	{
		if (!nes)
		{
			nes = new c_nes();
			nes->emulation_mode = emulation_mode;
			strcpy_s(nes->filename, MAX_PATH, filename.c_str());
			strcpy_s(nes->path, MAX_PATH, path.c_str());
			strcpy_s(nes->sram_path, MAX_PATH, sram_path.c_str());
			if (load)
				Load();
			if (nes->loaded)
				nes->disable_mixer();
		}
		is_active = 1;
	}
	++ref;
}

void Game::OnDeactivate()
{
	if (ref == 0)
		return;
	--ref;
	if (ref == 0)
	{
		DestroyThread();
		if (nes)
		{
			delete nes;
			nes = 0;
		}
		is_active = 0;
	}
}

void Game::InitEvents()
{
	eventStart = CreateEvent(NULL, FALSE, FALSE, NULL);
	eventDone = CreateEvent(NULL, FALSE, FALSE, NULL);
	eventKill = CreateEvent(NULL, FALSE, FALSE, NULL);
	h[0] = eventStart;
	h[1] = eventKill;
	//h[0] = g_start_event;
}

void Game::DestroyEvents()
{
	h[0] = 0;
	h[1] = 0;

	if (eventStart)
	{
		CloseHandle(eventStart);
		eventStart = 0;
	}
	if (eventDone)
	{
		CloseHandle(eventDone);
		eventDone = 0;
	}
	if (eventKill)
	{
		CloseHandle(eventKill);
		eventKill = 0;
	}
}

void Game::InitThread()
{
	return;
	if (threadHandle)
		return;
	InitEvents();
	threadHandle = CreateThread(NULL, 0, Game::NesThread, this, 0, NULL);
	running = true;
}

void Game::DestroyThread()
{
	return;
	if (!threadHandle)
		return;
	kill = true;
	running = false;
	SetEvent(eventKill);
	WaitForSingleObject(threadHandle, INFINITE);
	//while (threadHandle)
	//	Sleep(0);
	CloseHandle(threadHandle);
	threadHandle = 0;
}

void Game::DrawToTexture(ID3D10Texture2D *tex)
{
	D3D10_MAPPED_TEXTURE2D map;
	map.pData = 0;
	tex->Map(0, D3D10_MAP_WRITE_DISCARD, NULL, &map);
	int *p = 0;
	if (nes->loaded)
	{
		int *fb = nes->GetVideo();
		for (int y = 0; y < 240; ++y)
		{
			p = (int*)map.pData + (y) * (map.RowPitch / 4);
			for (int x = 0; x < 256; ++x)
			{
				if (mask_sides && (x < 8 || x > 247))
				{
					fb++;
					*p++ = 0xFF000000;
				}
				else
				{
					int col = pal[*fb++ & 0x1FF];
					*p++ = col;
				}
			}
		}

	}
	else
	{
		unsigned char c = 0;
		for (int y = 0; y < 240; ++y)
		{
			p = (int*)map.pData + y * (map.RowPitch / 4);
			for (int x = 0; x < 256; ++x)
			{
				c = rand() & 0xFF;
				//c *= c;
				//c = c > 255 ? 255 : c;
				*p++ = 0xFF << 24 | c << 16 | c << 8 | c;
				//*p++ = 0xFFFF0000;
			}
		}
	}
	tex->Unmap(0);
}

bool Game::Selectable()
{
	return nes->loaded;
}