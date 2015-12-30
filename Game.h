#pragma once
#include "console.h"
#include "nes\nes.h"
#include "sms\sms.h"
#include <string>
#include "TexturePanelItem.h"
#include "d3dx10.h"

class Game : public TexturePanelItem
{
public:
	Game(std::string type, std::string path, std::string filename, std::string sram_path);
	HANDLE GetEventStart() { return eventStart; } //Retrieve eventStart handle
	HANDLE GetEventDone() { return eventDone; } //Retrieve eventDone handle
	//void Load(); //Load nes and create thread
	//void Unload(); //Kill thread and unload nes
	~Game(void);
	HANDLE eventStart;
	HANDLE eventDone;
	c_console* console;
	void DrawToTexture(ID3D10Texture2D *tex);
	ID3D10Buffer *get_vertex_buffer() { return vertex_buffer; };
	bool Selectable();
	bool mask_sides;
	bool limit_sprites;
	int emulation_mode;
	std::string get_filename();
	bool favorite;
	int submapper;
	char title[MAX_PATH];
	std::string filename;
private:
	void OnActivate(bool load);
	void OnDeactivate();
	void OnLoad();
	void create_vertex_buffer();

	int ref;

	//std::string filename;
	std::string path;
	std::string sram_path;
	std::string type;

	struct SimpleVertex
	{
		D3DXVECTOR3 pos;
		D3DXVECTOR2 tex;
	};

	D3D10_BUFFER_DESC bd;
	ID3D10Buffer *vertex_buffer;
};
