#include "Game.h"
#include <crtdbg.h>
#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif
extern HANDLE g_start_event;

extern ID3D10Device *d3dDev;

void strip_extension(char *path);

std::string c_game::get_filename()
{
	return filename;
}

const c_game::SimpleVertex c_game::default_vertices[4] = {
		{ D3DXVECTOR3(-4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2(0.0f, 0.90625f) },
		{ D3DXVECTOR3(-4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2(0.0f, 0.03125f) },
		{ D3DXVECTOR3(4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2(1.0f, 0.90625f) },
		{ D3DXVECTOR3(4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2(1.0f, 0.03125f) },
};

c_game::c_game(GAME_TYPE type, std::string path, std::string filename, std::string sram_path) :
	mt(time(0))
{
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

}

c_game::~c_game()
{
	if (console)
		delete console;
	if (vertex_buffer)
	{
		vertex_buffer->Release();
		vertex_buffer = NULL;
	}
}

void c_game::OnLoad()
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

void c_game::OnActivate(bool load)
{
	if (ref == 0)
	{
		D3D10_SUBRESOURCE_DATA initData;
		initData.pSysMem = default_vertices;
		HRESULT hr = d3dDev->CreateBuffer(&bd, &initData, &default_vertex_buffer);

		if (!console)
		{
			switch (type)
			{
			case GAME_NES:
				console = new c_nes();
				break;
			case GAME_SMS:
				console = new c_sms(SMS_MODEL::SMS);
				break;
			case GAME_GG:
				console = new c_sms(SMS_MODEL::GAMEGEAR);
				break;
			case GAME_GB:
				console = new c_gb(GB_MODEL::DMG);
				break;
            case GAME_GBC:
                console = new c_gb(GB_MODEL::CGB);
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
				fb_width = console->get_fb_width();
				fb_height = console->get_fb_height();
			}
		}
		is_active = 1;
	}
	++ref;
}

void c_game::OnDeactivate()
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

void c_game::DrawToTexture(ID3D10Texture2D *tex)
{
	D3D10_MAPPED_TEXTURE2D map;
	map.pData = 0;
	tex->Map(0, D3D10_MAP_WRITE_DISCARD, NULL, &map);
	int* p;
	if (console && console->is_loaded())
	{
		int *fb = console->get_video();
		int y = 0;
		for (; y < fb_height; y++) {
			p = (int*)map.pData + (y) * (map.RowPitch / 4);
			int x = 0;
			for (; x < fb_width; x++) {
				*p++ = *fb++;
			}
			for (; x < 256; x++) {
				*p++ = 0xFF000000;
			}
		}
		for (; y < 256; y++) {
			p = (int*)map.pData + (y) * (map.RowPitch / 4);
			for (int x = 0; x < fb_width; x++) {
				*p++ = 0xFF000000;
			}
		}

	}
	else
	{
		unsigned char c = 0;
		for (int y = 0; y < 256; ++y)
		{
			p = (int*)map.pData + y * (map.RowPitch / 4);
			for (int x = 0; x < 256; ++x)
			{
				c = mt() & 0xFF;
				*p++ = 0xFF << 24 | c << 16 | c << 8 | c;
			}
		}
	}
	tex->Unmap(0);
}

bool c_game::Selectable()
{
	if (console)
		return console->is_loaded();
	else
		return 0;
}

void c_game::create_vertex_buffer()
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
	else if (type == GAME_GB || type == GAME_GBC)
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


	D3D10_SUBRESOURCE_DATA initData;
	initData.pSysMem = vertices;
	HRESULT hr = d3dDev->CreateBuffer(&bd, &initData, &vertex_buffer);
}

//TODO: this should probably be handled by console

int c_game::get_width()
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
    case GAME_GBC:
		return 160;
	default:
		return 256;
	}
}

int c_game::get_height()
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
    case GAME_GBC:
		return 144;
	default:
		return 256;
	}
}

ID3D10Buffer* c_game::get_vertex_buffer(int stretched)
{
	if (console && !console->is_loaded()) {
		return default_vertex_buffer;
	}
	if (stretched) return stretched_vertex_buffer; else return vertex_buffer;
};