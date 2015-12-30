#include "Game.h"
#include <crtdbg.h>
//#if defined(DEBUG) | defined(_DEBUG)
//#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
//#define new DEBUG_NEW
//#endif
extern int pal[64 * 8];
extern HANDLE g_start_event;

extern ID3D10Device *d3dDev;

void strip_extension(char *path);

std::string Game::get_filename()
{
	return filename;
}

Game::Game(std::string type, std::string path, std::string filename, std::string sram_path)
{
	emulation_mode = c_nes::EMULATION_MODE_FAST;
	limit_sprites = false;
	this->path = path;
	this->filename = filename;
	this->sram_path = sram_path;
	this->type = type;
	ref = 0;
	console = 0;
	mask_sides = false;
	favorite = false;
	submapper = 0;
	strcpy_s(title, filename.c_str());
	strip_extension(title);
}

Game::~Game(void)
{
	if (console)
		delete console;
}

void Game::OnLoad()
{
	if (!console->is_loaded())
	{
		console->load();
		if (console->is_loaded())
		{
			//nes->set_sprite_limit(limit_sprites);
		}
	}
}

void Game::OnActivate(bool load)
{
	if (ref == 0)
	{
		if (!console)
		{
			if (type == "nes")
			{
				console = new c_nes();
			}
			else if (type == "sms")
			{
				console = new c_sms();
			}
			if (console)
			{
				console->set_emulation_mode(emulation_mode);
				strcpy_s(console->filename, MAX_PATH, filename.c_str());
				strcpy_s(console->path, MAX_PATH, path.c_str());
				strcpy_s(console->sram_path, MAX_PATH, sram_path.c_str());
				if (load)
					console->load();
				if (console->is_loaded())
					console->disable_mixer();
				create_vertex_buffer();
			}
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
		if (console)
		{
			delete console;
			console = 0;
		}
		is_active = 0;
	}
}

void Game::DrawToTexture(ID3D10Texture2D *tex)
{
	D3D10_MAPPED_TEXTURE2D map;
	map.pData = 0;
	tex->Map(0, D3D10_MAP_WRITE_DISCARD, NULL, &map);
	int *p = 0;
	if (console && console->is_loaded())
	{
		int *fb = console->get_video();
		if (type == "nes")
		{
			for (int y = 0; y < 240; ++y)
			{
				p = (int*)map.pData + (y)* (map.RowPitch / 4);
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
		else if (type == "sms")
		{
			int y = 0;
			for (; y < 24; y++)
			{
				p = (int*)map.pData + (y)* (map.RowPitch / 4);
				for (int x = 0; x < 256; x++)
					*p++ = 0xFF000000;
			}
			for (; y < 192+24; ++y)
			{
				p = (int*)map.pData + (y)* (map.RowPitch / 4);
				for (int x = 0; x < 256; ++x)
				{
					*p++ = *fb++;
				}
			}
			for (; y < 256; ++y)
			{
				p = (int*)map.pData + (y)* (map.RowPitch / 4);
				for (int x = 0; x < 256; ++x)
					*p++ = 0xFF000000;
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
				*p++ = 0xFF << 24 | c << 16 | c << 8 | c;
			}
		}
	}
	tex->Unmap(0);
}

bool Game::Selectable()
{
	if (console)
		return console->is_loaded();
	else
		return 0;
}

void Game::create_vertex_buffer()
{
	SimpleVertex vertices[] =
	{
		{ D3DXVECTOR3(-4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2(0.0f, 0.90625f) },
		{ D3DXVECTOR3(-4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2(0.0f, 0.03125f) },
		{ D3DXVECTOR3(4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2(1.0f, 0.90625f) },
		{ D3DXVECTOR3(4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2(1.0f, 0.03125f) },
	};

	//if (type == "sms")
	//{
	//	vertices[0].tex = D3DXVECTOR2(0.0f, .805f);
	//	vertices[1].tex = D3DXVECTOR2(0.0f, -.055f);
	//	vertices[2].tex = D3DXVECTOR2(1.0f, .805f);
	//	vertices[3].tex = D3DXVECTOR2(1.0f, -.055f);

	//}

	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 4;
	bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	D3D10_SUBRESOURCE_DATA initData;
	initData.pSysMem = vertices;
	HRESULT hr = d3dDev->CreateBuffer(&bd, &initData, &vertex_buffer);
}