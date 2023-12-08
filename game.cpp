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
		SimpleVertex unloaded_vertices[4] = {{D3DXVECTOR3(-4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2(0.0f, (double)static_height/tex_height)},
                                             {D3DXVECTOR3(-4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2(0.0f, 0.0f)},
                                             {D3DXVECTOR3(4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2((double)static_width/tex_width, (double)static_height/tex_height)},
                                             {D3DXVECTOR3(4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2((double)static_width/tex_width, 0.0f)}};

        D3D10_SUBRESOURCE_DATA initData;
		initData.pSysMem = unloaded_vertices;
        HRESULT hr = d3dDev->CreateBuffer(&bd, &initData, &unloaded_vertex_buffer);

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
            case GAME_PACMAN:
                console = new c_pacman();
                break;
            case GAME_MSPACMAN:
                console = new c_mspacman();
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
			for (; x < tex_width; x++) {
				*p++ = 0xFF000000;
			}
		}
		for (; y < tex_height; y++) {
			p = (int*)map.pData + (y) * (map.RowPitch / 4);
			for (int x = 0; x < tex_width; x++) {
				*p++ = 0xFF000000;
			}
		}

	}
	else
	{
		unsigned char c = 0;
		for (int y = 0; y < static_height; ++y)
		{
			p = (int*)map.pData + y * (map.RowPitch / 4);
			for (int x = 0; x < static_width; ++x)
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

	//width and height of texture we're drawing to

	int h = console->get_fb_height();
    int w = console->get_fb_width();
    auto [crop_l, crop_r, crop_t, crop_b] = console->get_crop();

	double w_adjust = 0.0;
    double h_adjust = 0.0;
	
	double aspect_ratio = console->get_aspect_ratio();

    if (aspect_ratio != 0.0) {
        if (aspect_ratio > 1.0) {
            w_adjust = ((4.0 / 3.0) / aspect_ratio * w - w) / 2.0;
        }
        else {
            h_adjust = ((4.0 / 3.0) / aspect_ratio * h - h) / 2.0;
        }
    }

	double w_start = (crop_l - w_adjust);
    double w_end = (w - crop_r + w_adjust);
    double h_start = (crop_t - h_adjust);
    double h_end = (h - crop_b + h_adjust);
    width = w_end - w_start;
    height = h_end - h_start;

	w_start /= tex_width;
    w_end /= tex_width;
    h_start /= tex_height;
    h_end /= tex_height;

	SimpleVertex vertices[4] = {
        {D3DXVECTOR3(-4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2(w_start, h_end)},
        {D3DXVECTOR3(-4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2(w_start, h_start)},
        {D3DXVECTOR3(4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2(w_end, h_end)},
        {D3DXVECTOR3(4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2(w_end, h_start)},
    };

	if (console->is_rotated()) {
        vertices[0].tex.x = vertices[2].tex.x = w_end;
        vertices[1].tex.x = vertices[3].tex.x = w_start;
        vertices[0].tex.y = vertices[1].tex.y = h_end;
        vertices[2].tex.y = vertices[3].tex.y = h_start;
    }

	D3D10_SUBRESOURCE_DATA initData;
	initData.pSysMem = vertices;
	HRESULT hr = d3dDev->CreateBuffer(&bd, &initData, &vertex_buffer);
}

//return the pixel width/height accounting for cropping and aspect ratio adjustment
//this is the number of visible pixels in the texture.  It's needed by the shader
//for scaling purposes.
double c_game::get_width()
{
    if (console && !console->is_loaded()) {
        return static_width;
    }
    return width;
}

double c_game::get_height()
{
    if (console && !console->is_loaded()) {
        return static_height;
    }
    return height;
}

ID3D10Buffer* c_game::get_vertex_buffer(int stretched)
{
	if (console && !console->is_loaded()) {
		return unloaded_vertex_buffer;
	}
	if (stretched) return stretched_vertex_buffer; else return vertex_buffer;
};