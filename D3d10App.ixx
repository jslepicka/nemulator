module;

#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif

#ifndef _WIN64
#error This application requires x64 compilation
#endif

#define WIN32_LEAN_AND_MEAN

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#endif

#include <d3d10.h>
#include <dxgi.h>
#include <dxerr.h>
#include <d3dx10.h>
#include <windows.h>

export module D3d10App;
import nemulator.std;
import input_handler;
import config;
import task;

export class D3d10App
{
public:
	D3d10App(HINSTANCE hInstance);
	virtual ~D3d10App();
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

	//fullscreen can be changed from a task; the change is made before the next frame
	static bool is_fullscreen();
	static void set_fullscreen(bool enable);
	static constexpr bool default_fullscreen = true;
	//sets and saves the windowed mode width; the window is resized once it's windowed.  returns false if
	//it couldn't be saved.
	static bool set_window_width(int width);
	static constexpr int default_window_width = 0; //uses default_window_scale
	static constexpr double default_window_scale = .5; //fraction of the screen's work area width
	static constexpr bool default_aspect_lock = true;
	static constexpr bool default_vsync = true;
	static constexpr bool default_timer_sync = false;
	static constexpr bool default_pause_on_lost_focus = true;

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
	bool resized;
	void disable_screensaver();

	HINSTANCE hInstance;

	bool paused;
	bool minimized;
	bool maximized;
	bool resizing;
	bool vsync;
	bool timer_sync;
	bool pause_on_lost_focus;
	int ignore_input;

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
	int fullscreen_request; //-1 = no change requested
	static D3d10App *instance;
	int window_width; //windowed mode client width, as saved in app.x
	bool window_width_pending; //the window hasn't been resized to window_width yet
	bool save_window_width();
	static int get_window_width(int configured);
	void resize_window(int width);

	DXGI_MODE_DESC matching_mode;
	DXGI_SWAP_CHAIN_DESC sd;
	IDXGIFactory * pFactory;

	HANDLE power_request;
    D3D10_TEXTURE2D_DESC depthStencilDesc;
};
