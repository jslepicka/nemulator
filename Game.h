#pragma once
#include "nes\nes.h"
#include <string>
#include "TexturePanelItem.h"

class Game : public TexturePanelItem
{
public:
	Game(std::string path, std::string filename, std::string sram_path);
	HANDLE GetEventStart() { return eventStart; } //Retrieve eventStart handle
	HANDLE GetEventDone() { return eventDone; } //Retrieve eventDone handle
	//void Load(); //Load nes and create thread
	//void Unload(); //Kill thread and unload nes
	~Game(void);
	HANDLE eventStart;
	HANDLE eventDone;
	c_nes* nes;
	void DrawToTexture(ID3D10Texture2D *tex);
	bool Selectable();
	bool mask_sides;
	bool limit_sprites;
	int emulation_mode;
	std::string get_filename();
	bool favorite;
	int submapper;
private:
	void InitThread();
	void DestroyThread();
	void InitEvents();
	void DestroyEvents();
	void OnActivate(bool load);
	void OnDeactivate();
	void OnLoad();

	int ref;

	static DWORD WINAPI NesThread(LPVOID lpParam);

	
	bool running;
	bool kill;
	void* threadHandle;
	HANDLE eventKill;
	HANDLE h[2];
	std::string filename;
	std::string path;
	std::string sram_path;
};
