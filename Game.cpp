#include "Game.h"
#include <crtdbg.h>
#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif
extern int pal[64 * 8];
extern HANDLE g_start_event;

extern ID3D10Device *d3dDev;

void strip_extension(char *path);

std::string Game::get_filename()
{
	return filename;
}

const Game::SimpleVertex Game::default_vertices[4] = {
		{ D3DXVECTOR3(-4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2(0.0f, 0.90625f) },
		{ D3DXVECTOR3(-4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2(0.0f, 0.03125f) },
		{ D3DXVECTOR3(4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2(1.0f, 0.90625f) },
		{ D3DXVECTOR3(4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2(1.0f, 0.03125f) },
};

Game::Game(GAME_TYPE type, std::string path, std::string filename, std::string sram_path)
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

	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 4;
	bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	D3D10_SUBRESOURCE_DATA initData;
	initData.pSysMem = default_vertices;
	HRESULT hr = d3dDev->CreateBuffer(&bd, &initData, &default_vertex_buffer);
}

Game::~Game(void)
{
	if (console)
		delete console;
	if (vertex_buffer)
	{
		vertex_buffer->Release();
		vertex_buffer = NULL;
	}
}

void Game::OnLoad()
{
	if (!console->is_loaded())
	{
		console->load();
		if (type == GAME_NES && console->is_loaded())
		{
			console->set_sprite_limit(limit_sprites);
		}
	}
}

void Game::OnActivate(bool load)
{
	if (ref == 0)
	{
		if (!console)
		{
			switch (type)
			{
			case GAME_NES:
				console = new c_nes();
				break;
			case GAME_SMS:
				console = new c_sms(0);
				break;
			case GAME_GG:
				console = new c_sms(1);
				break;
			case GAME_GB:
				console = new c_gb();
				break;
			default:
				break;
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
		if (vertex_buffer)
		{
			vertex_buffer->Release();
			vertex_buffer = NULL;
		}
		if (stretched_vertex_buffer)
		{
			stretched_vertex_buffer->Release();
			stretched_vertex_buffer = NULL;
		}
		if (default_vertex_buffer)
		{
			default_vertex_buffer->Release();
			default_vertex_buffer = NULL;
		}
		is_active = 0;
		played = 0;
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
		switch (type)
		{
		case GAME_NES:
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
			break;
		case GAME_SMS:
		case GAME_GG:
		{
			int y = 0;
			//for (; y < 14; y++)
			//{
			//	p = (int*)map.pData + (y)* (map.RowPitch / 4);
			//	for (int x = 0; x < 256; x++)
			//		*p++ = overscan;
			//}
			for (y = 0; y < 192+14; ++y)
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
		break;
		case GAME_GB:
		{
			int y = 0;
			for (y = 0; y < 144; ++y) {
				int x = 0;
				p = (int*)map.pData + (y) * (map.RowPitch / 4);
				for (x = 0; x < 160; ++x) {
					*p++ = *fb++;
				}
				for (; x < 256; ++x) {
					*p++ = 0xFF000000;
				}
			}
			for (; y < 256; y++) {
				p = (int*)map.pData + (y) * (map.RowPitch / 4);
				for (int x = 0; x < 256; ++x) {
					*p++ = 0xFF000000;
				}
			}
		}
		break;
		default:
			break;
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
	if (vertex_buffer)
	{
		vertex_buffer->Release();
		vertex_buffer = NULL;
	}

	SimpleVertex vertices[4];
	memcpy(vertices, default_vertices, sizeof(default_vertices));

	if (type == GAME_SMS)
	{
		vertices[0].tex.y = vertices[2].tex.y = (192.0 + 14.0) / 256.0;
		vertices[1].tex.y = vertices[3].tex.y = -14.0 / 256.0;
	}
	else if (type == GAME_GG)
	{
		vertices[0].tex.y = vertices[2].tex.y = (144.0 + 24.0) / 256.0;
		vertices[1].tex.y = vertices[3].tex.y = 24.0 / 256.0;

		vertices[0].tex.x = vertices[1].tex.x = 48.0 / 256.0;
		vertices[2].tex.x = vertices[3].tex.x = (256.0 - 48.0) / 256.0;
	}
	else if (type == GAME_GB)
	{
		vertices[0].tex.y = vertices[2].tex.y = 144.0 / 256.0;
		vertices[1].tex.y = vertices[3].tex.y = 0.0 / 256.0;

		//aspect ratio adjustment
		//gb ratio = 4.7f/4.3f
		//(((1.3333 / 1.093) * 160) - 160) / 2
		auto adjust = (((4.0 / 3.0) / (4.7 / 4.3) * 160.0) - 160.0) / 2.0;
		vertices[0].tex.x = vertices[1].tex.x = -adjust / 256.0;
		vertices[2].tex.x = vertices[3].tex.x = (256.0 - 96.0 + adjust) / 256.0;
	}


	memcpy(vertices2, vertices, sizeof(vertices2));

	D3D10_SUBRESOURCE_DATA initData;
	initData.pSysMem = vertices;
	HRESULT hr = d3dDev->CreateBuffer(&bd, &initData, &vertex_buffer);
}

void Game::build_stretch_buffer(float ratio)
{
	if (stretched_vertex_buffer)
	{
		stretched_vertex_buffer->Release();
		stretched_vertex_buffer = NULL;
	}
	vertices2[0].pos.x = -ratio;
	vertices2[1].pos.x = -ratio;
	vertices2[2].pos.x = ratio;
	vertices2[3].pos.x = ratio;

	D3D10_SUBRESOURCE_DATA initData;
	initData.pSysMem = vertices2;
	HRESULT hr = d3dDev->CreateBuffer(&bd, &initData, &stretched_vertex_buffer);
}

D3DXCOLOR Game::get_overscan_color()
{
	if (console->is_loaded())
	{
		float r, g, b, a;
		switch (type)
		{
		case GAME_SMS:
		case GAME_GG:
			r = (console->get_overscan_color() & 0xFF) / 255.0;
			g = ((console->get_overscan_color() >> 8) & 0xFF) / 255.0;
			b = ((console->get_overscan_color() >> 16) & 0xFF) / 255.0;
			a = ((console->get_overscan_color() >> 24) & 0xFF) / 255.0;
			return D3DXCOLOR(r, g, b, a);
			break;
		default:
			return D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
		}
	}
	else
	{
		return D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f);
	}
}

//TODO: this should probably be handled by console

int Game::get_width()
{
	switch (type)
	{
	case GAME_NES:
		return 256;
	case GAME_SMS:
		return 256;
	case GAME_GG:
		return 160;
	case GAME_GB:
		return 160;
	default:
		return 256;
	}
}

int Game::get_height()
{
	switch (type)
	{
	case GAME_NES:
		return 224;
	case GAME_SMS:
		return 192 + 28;
	case GAME_GG:
		return 144;
	case GAME_GB:
		return 144;
	default:
		return 256;
	}
}

ID3D10Buffer* Game::get_vertex_buffer(int stretched)
{
	if (console && !console->is_loaded()) {
		return default_vertex_buffer;
	}
	if (stretched) return stretched_vertex_buffer; else return vertex_buffer;
};