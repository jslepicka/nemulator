#pragma once

#pragma comment (lib, "d3d10.lib")
#pragma comment (lib, "d3dx10.lib")
#pragma comment (lib, "dxerr.lib")
#pragma comment (lib, "dxgi.lib")

//compile for 7+
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#endif

#define ReleaseCOM(x) { if(x) {x->Release(); x = 0; } }

#include <d3d10.h>
#include <dxgi.h>
#include <dxerr.h>
#include <d3dx10.h>
#include <windows.h>
#include <string>
#include "input_handler.h"
#include "config.h"
#include <stack>
#include "task.h"
#include <deque>
#include "task1.h"
#include <vector>
#include <string>

class D3d10App
{
public:
	D3d10App(HINSTANCE hInstance);
	virtual ~D3d10App(void);
	HINSTANCE GetInstance();
	HWND GetWnd();
	int Run();

	virtual void Init(char *config_file_name, c_task *init_task, void *params);
	virtual void OnResize();

	virtual void UpdateScene(float dt);
	virtual void DrawScene();
	virtual void OnKeyUp(WPARAM wParam);
	virtual LRESULT MsgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	void SetCaption(std::string cap);

	struct SimpleVertex
	{
		D3DXVECTOR3 pos;
		D3DXVECTOR2 tex;
	};
	D3DXCOLOR clearColor;

protected:
	void InitWnd();
	int InitD3d();
	virtual void OnPause(bool paused);
	void add_task(c_task *task, void *params);
	bool resized;
	void disable_screensaver();

	HINSTANCE hInstance;

	bool paused;
	bool minimized;
	bool maximized;
	bool resizing;
	bool vsync;
	bool timer_sync;
	//bool startFullscreen;

	//ID3D10Device *d3dDev;
	IDXGISwapChain *swapChain;
	ID3D10Texture2D *depthStencilBuffer;
	ID3D10RenderTargetView *renderTargetView;
	ID3D10DepthStencilView *depthStencilView;

	//define these in derived class constructor
	std::string caption;
	D3D10_DRIVER_TYPE d3dDriverType;


	LARGE_INTEGER liFreq;
	LARGE_INTEGER liPrev;
	LARGE_INTEGER liCurrent;



	int window_adj_x;
	int window_adj_y;

	DWORD task_index;
	HANDLE avrt_handle;

	BOOL fullscreen;

	DXGI_MODE_DESC matching_mode;
	DXGI_SWAP_CHAIN_DESC sd;
	IDXGIFactory * pFactory;

	HANDLE power_request;
};
