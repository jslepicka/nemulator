#include "Game.h"
#include <crtdbg.h>
#include <immintrin.h>

extern HANDLE g_start_event;

extern ID3D10Device *d3dDev;

void strip_extension(char *path);

std::string c_game::get_filename()
{
	return filename;
}

c_game::c_game(GAME_TYPE type, std::string path, std::string filename, std::string sram_path) :
	mt((unsigned int)time(0))
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
	/*if (console)
		delete console;*/
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
		SimpleVertex unloaded_vertices[4] = {{D3DXVECTOR3(-4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2(0.0f, (FLOAT)((double)static_height/tex_height))},
                                             {D3DXVECTOR3(-4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2(0.0f, 0.0f)},
                                             {D3DXVECTOR3(4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2((FLOAT)((double)static_width/tex_width), (FLOAT)((double)static_height/tex_height))},
                                             {D3DXVECTOR3(4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2((FLOAT)((double)static_width/tex_width), 0.0f)}};

        D3D10_SUBRESOURCE_DATA initData;
		initData.pSysMem = unloaded_vertices;
        HRESULT hr = d3dDev->CreateBuffer(&bd, &initData, &unloaded_vertex_buffer);

		if (!console)
		{
			switch (type)
			{
			case GAME_NES:
                console = std::make_unique<c_nes>();
				break;
			case GAME_SMS:
                console = std::make_unique<c_sms>(SMS_MODEL::SMS);
				break;
			case GAME_GG:
                console = std::make_unique<c_sms>(SMS_MODEL::GAMEGEAR);
				break;
			case GAME_GB:
                console = std::make_unique<c_gb>(GB_MODEL::DMG);
				break;
            case GAME_GBC:
                console = std::make_unique<c_gb>(GB_MODEL::CGB);
                break;
            case GAME_PACMAN:
                console = std::make_unique<c_pacman>();
                break;
            case GAME_MSPACMAN:
                console = std::make_unique<c_mspacman>();
                break;
			default:
				break;
			}
			if (console)
			{
				console->get_display_info(&display_info);
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

void c_game::OnDeactivate()
{
	if (ref == 0)
		return;
	--ref;
	if (ref == 0)
	{
		if (console)
		{
			//delete console;
            console.reset();
			//console = 0;
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
		for (; y < display_info.fb_height; y++) {
			p = (int*)map.pData + (y) * (map.RowPitch / 4);
			int x = 0;
            //constexpr int memcpy_block_size = 64;
            //for (; x < display_info.fb_width / memcpy_block_size; x += memcpy_block_size) {
            //    memcpy(p, fb, memcpy_block_size * sizeof(int));
            //    p += memcpy_block_size;
            //    fb += memcpy_block_size;
            //}
			for (; x < display_info.fb_width; x++) {
				*p++ = *fb++;
			}
			for (; x < tex_width; x++) {
				*p++ = 0xFF000000;
			}
		}
		for (; y < tex_height; y++) {
			p = (int*)map.pData + (y) * (map.RowPitch / 4);
            for (int x = 0; x < tex_width; x++) {
				*p++ = 0xff000000;
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
	int h = display_info.fb_height;
    int w = display_info.fb_width;

	double w_adjust = 0.0;
    double h_adjust = 0.0;

	double aspect_ratio = display_info.aspect_ratio;

    if (aspect_ratio != 0.0) {
        if (aspect_ratio > 1.0) {
            w_adjust = ((4.0 / 3.0) / aspect_ratio * w - w) / 2.0;
        }
        else {
            h_adjust = ((4.0 / 3.0) / aspect_ratio * h - h) / 2.0;
        }
    }

	double w_start = (display_info.crop_left - w_adjust);
    double w_end = (w - display_info.crop_right + w_adjust);
    double h_start = (display_info.crop_top - h_adjust);
    double h_end = (h - display_info.crop_bottom + h_adjust);
    width = w_end - w_start;
    height = h_end - h_start;

	w_start /= tex_width;
    w_end /= tex_width;
    h_start /= tex_height;
    h_end /= tex_height;

	SimpleVertex vertices[4] = {
        {D3DXVECTOR3(-4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2((FLOAT)w_start, (FLOAT)h_end)},
        {D3DXVECTOR3(-4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2((FLOAT)w_start, (FLOAT)h_start)},
        {D3DXVECTOR3(4.0f / 3.0f, -1.0f, 0.0f), D3DXVECTOR2((FLOAT)w_end, (FLOAT)h_end)},
        {D3DXVECTOR3(4.0f / 3.0f, 1.0f, 0.0f), D3DXVECTOR2((FLOAT)w_end, (FLOAT)h_start)},
    };

	if (display_info.rotated) {
        vertices[0].tex.x = vertices[2].tex.x = (FLOAT)w_end;
        vertices[1].tex.x = vertices[3].tex.x = (FLOAT)w_start;
        vertices[0].tex.y = vertices[1].tex.y = (FLOAT)h_end;
        vertices[2].tex.y = vertices[3].tex.y = (FLOAT)h_start;
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