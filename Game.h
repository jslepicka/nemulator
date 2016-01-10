#pragma once
#include "console.h"
#include "nes\nes.h"
#include "sms\sms.h"
#include <string>
#include "TexturePanelItem.h"
#include "d3dx10.h"

enum GAME_TYPE
{
	GAME_NES,
	GAME_SMS,
	GAME_NONE
};

class Game : public TexturePanelItem
{
public:
	Game(GAME_TYPE type, std::string path, std::string filename, std::string sram_path);
	HANDLE GetEventStart() { return eventStart; } //Retrieve eventStart handle
	HANDLE GetEventDone() { return eventDone; } //Retrieve eventDone handle
	//void Load(); //Load nes and create thread
	//void Unload(); //Kill thread and unload nes
	~Game(void);
	HANDLE eventStart;
	HANDLE eventDone;
	c_console* console;
	void DrawToTexture(ID3D10Texture2D *tex);
	ID3D10Buffer *get_vertex_buffer(int stretched) { if (stretched) return stretched_vertex_buffer; else return vertex_buffer; };
	void build_stretch_buffer(float ratio);
	bool Selectable();
	bool mask_sides;
	bool limit_sprites;
	int emulation_mode;
	std::string get_filename();
	bool favorite;
	int submapper;
	char title[MAX_PATH];
	std::string filename;
	GAME_TYPE type;
	int played = 0;
	D3DXCOLOR get_overscan_color();
	int get_height();
private:
	void OnActivate(bool load);
	void OnDeactivate();
	void OnLoad();
	void create_vertex_buffer();

	int ref;

	//std::string filename;
	std::string path;
	std::string sram_path;

	struct SimpleVertex
	{
		D3DXVECTOR3 pos;
		D3DXVECTOR2 tex;
	};
	SimpleVertex vertices2[4];
	D3D10_BUFFER_DESC bd;
	ID3D10Buffer *vertex_buffer = NULL;
	ID3D10Buffer *stretched_vertex_buffer = NULL;
};
