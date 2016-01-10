#include "D3d10App.h"
#include "nemulator.h"
#include "avrt.h"
#include "dwmapi.h"
#include <algorithm>

#pragma comment (lib, "PowrProf.lib")
#pragma comment (lib, "avrt.lib")
#pragma comment (lib, "dwmapi.lib")

ID3D10Device *d3dDev;
D3DXMATRIX matrixView;
D3DXMATRIX matrixProj;
int clientHeight;
int clientWidth;
bool startFullscreen;
bool aspectLock;
double aspectRatio;
HWND hWnd;
c_config *config = NULL;
c_input_handler *g_ih = NULL;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static D3d10App *app = 0;
	switch (msg)
	{
	case WM_CREATE:
		{
			CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
			app = (D3d10App*)cs->lpCreateParams;
			return 0;
		}
	}

	if (app)
		return app->MsgProc(msg, wParam, lParam);
	else
		return DefWindowProc(hWnd, msg, wParam, lParam);
}

D3d10App::D3d10App(HINSTANCE hInstance)
{
	fullscreen = FALSE;
	this->hInstance = hInstance;
	hWnd = 0;
	paused = false;
	minimized = false;
	maximized = false;
	resizing = false;
	resized = false;
	startFullscreen = false;

	d3dDev = 0;
	swapChain = 0;
	depthStencilBuffer = 0;
	renderTargetView = 0;
	depthStencilView = 0;

	caption = "";
	d3dDriverType = D3D10_DRIVER_TYPE_HARDWARE;
	clearColor = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
	clientWidth = 960;
	clientHeight = 600;

	aspectLock = false;
	aspectRatio = 0.0;

	config = NULL;
	g_ih = NULL;
}

D3d10App::~D3d10App(void)
{
	timeEndPeriod(1);
	if (avrt_handle != 0)
	{
		AvRevertMmThreadCharacteristics(avrt_handle);
	}
	DwmEnableMMCSS(FALSE);
	
	if (swapChain)
		swapChain->SetFullscreenState(false, NULL);

	KillTimer(hWnd, 0);
	if (d3dDev)
		d3dDev->ClearState();

	ReleaseCOM(renderTargetView);
	ReleaseCOM(depthStencilView);
	ReleaseCOM(swapChain);
	ReleaseCOM(depthStencilBuffer);
	ReleaseCOM(d3dDev);

	if (config)
		delete config;
	if (g_ih)
		delete g_ih;
}

HINSTANCE D3d10App::GetInstance()
{
	return hInstance;
}

HWND D3d10App::GetWnd()
{
	return hWnd;
}

void D3d10App::OnPause(bool paused)
{
	for (auto &task : *c_task::task_list)
	{
		task->on_pause(paused);
	}

}

int D3d10App::Run()
{
	MSG msg = {0};
	double dt = 0.0;
	while (msg.message != WM_QUIT && c_task::task_list->size() > 0)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if (!paused)
			{
				g_ih->poll(dt);
				d3dDev->ClearRenderTargetView(renderTargetView, clearColor);
				d3dDev->ClearDepthStencilView(depthStencilView, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 1.0f, 0);
				int r = 999;
				int params = 0;
				for (auto i = c_task::task_list->begin(); i != c_task::task_list->end();/* ++i*/)
				{
					auto task = *i;
					if (resized)
						task->resize();
					r = task->update(dt, r, &params);
					if (task->dead)
					{
						delete task;
						i = c_task::task_list->erase(i);
					}
					else
						++i;
				}


				std::for_each(c_task::task_list->rbegin(), c_task::task_list->rend(), [](c_task* task) {
					task->draw();
				});

				d3dDev->Flush();
				
				swapChain->Present(vsync ? 1 : 0, 0);
			}
			else
				Sleep(250);

			if (timer_sync)
			{
				double refresh_rate = 1000.0/59.97;
				for (;;)
				{
					QueryPerformanceCounter(&liCurrent);
					double elapsed = ((liCurrent.QuadPart - liPrev.QuadPart) * 1000) / (double)liFreq.QuadPart;
					if (elapsed >= refresh_rate)
					{
						break;
					}
					else
					{
						if (refresh_rate - elapsed > 2)
							Sleep(1);
						else
							Sleep(0);
					}

				}
			}
			else
				QueryPerformanceCounter(&liCurrent);

			dt = ((liCurrent.QuadPart - liPrev.QuadPart) * 1000 / (float)liFreq.QuadPart);
			liPrev = liCurrent;
			resized = false;

		}
	}
	for (auto i = c_task::task_list->begin(); i != c_task::task_list->end();)
	{
		auto task = *i;
		delete task;
		i = c_task::task_list->erase(i);
	}
	delete c_task::task_list;
	return (int)msg.wParam;
}

void D3d10App::Init(char *config_file_name, c_task *init_task, void *params)
{
	config = new c_config();
	config->read_config_file(config_file_name);
	clientWidth = config->get_int("app.x", 640);
	if (clientWidth < 2)
		clientWidth = 2;
	aspectRatio = config->get_bool("app.widescreen", true) ? 16.0/10.0 : (4.0/3.0);
	aspectLock = config->get_bool("app.aspect_lock", true);
	startFullscreen = config->get_bool("app.fullscreen", true);
	vsync = config->get_bool("app.vsync", true);
	timer_sync = config->get_bool("app.timer_sync", false);
	avrt_handle = 0;
	timeBeginPeriod(1);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	task_index = 0;

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	avrt_handle = AvSetMmThreadCharacteristics("Pro Audio", &task_index);
	clientHeight = (int)(clientWidth / aspectRatio);

	fullscreen = startFullscreen;
	InitWnd();
	if (!InitD3d())
		return;

	if (swapChain)
	{
		swapChain->SetFullscreenState(fullscreen, NULL);
		DXGI_MODE_DESC mode = { 0 };
		mode.Width = clientWidth;
		mode.Height = clientHeight;
		swapChain->ResizeTarget(&mode);
	}
	DwmEnableMMCSS(!fullscreen);
	if (fullscreen)
		ShowCursor(FALSE);

	SetTimer(hWnd, 0, 30000, NULL);

	c_task::add_task(init_task, params);

	QueryPerformanceFrequency(&liFreq);
	QueryPerformanceCounter(&liPrev);
	QueryPerformanceCounter(&liCurrent);
}

void D3d10App::OnResize()
{
	d3dDev->OMSetRenderTargets(0, 0, 0);
	ReleaseCOM(renderTargetView);
	ReleaseCOM(depthStencilView);
	ReleaseCOM(depthStencilBuffer);
	d3dDev->ClearState();
	swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	DXGI_MODE_DESC mode = { 0 };
	mode.Width = clientWidth;
	mode.Height = clientHeight;

	swapChain->ResizeTarget(&mode);

	ID3D10Texture2D *backBuffer;
	swapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (void **)(&backBuffer));
	d3dDev->CreateRenderTargetView(backBuffer, 0, &renderTargetView);
	ReleaseCOM(backBuffer);

	D3D10_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = clientWidth;
	depthStencilDesc.Height = clientHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D10_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	d3dDev->CreateTexture2D(&depthStencilDesc, 0, &depthStencilBuffer);
	d3dDev->CreateDepthStencilView(depthStencilBuffer, 0, &depthStencilView);

	d3dDev->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

	D3D10_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = clientWidth;
	vp.Height = clientHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	d3dDev->RSSetViewports(1, &vp);
	resized = true;
}

void D3d10App::UpdateScene(float dt)
{
}

void D3d10App::DrawScene()
{

}

LRESULT D3d10App::MsgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			paused = true;
			OnPause(paused);
		}
		else
		{
			paused = false;
			OnPause(paused);
		}
		return 0;
	case WM_SIZE:
		clientWidth = LOWORD(lParam);
		clientHeight = HIWORD(lParam);
		if (d3dDev)
		{
			BOOL fs;
			if (SUCCEEDED(swapChain->GetFullscreenState(&fs, NULL)))
			{
				if (fs != fullscreen)
				{
					fullscreen = fs;
					DwmEnableMMCSS(!fullscreen);
					ShowCursor(!fullscreen);
				}
			}
			if (wParam == SIZE_MINIMIZED)
			{
				if (!paused)
				{
					paused = true;
					OnPause(paused);
				}
				minimized = true;
				maximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				if (paused)
				{
					paused = false;
					OnPause(paused);
				}
				minimized = false;
				maximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				if (minimized)
				{
					if (paused)
					{
						paused = false;
						OnPause(paused);
					}
					minimized = false;
					OnResize();
				}
				else if (maximized)
				{
					if (paused)
					{
						paused = false;
						OnPause(paused);
					}
					maximized = false;
					OnResize();
				}
				else if (resizing)
				{
				}
				else
				{
					OnResize();
				}
			}
		}
		return 0;

	case WM_ENTERSIZEMOVE:
		if (!paused)
		{
			paused = true;
			OnPause(paused);
		}
		resizing = true;
		return 0;

	case WM_EXITSIZEMOVE:
		if (paused)
		{
			paused = false;
			OnPause(paused);
		}
		resizing = false;
		OnResize();
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);

	case WM_GETMINMAXINFO:
		{
			((MINMAXINFO*)lParam)->ptMinTrackSize.x = window_adj_x + 1;
			((MINMAXINFO*)lParam)->ptMinTrackSize.y = window_adj_y + 1;

		}
		return 0;

	case WM_SIZING:
		if (aspectLock && aspectRatio > 0.0)
		{

			RECT *r = (RECT *)lParam;
			int w = r->right - r->left - window_adj_x;
			r->bottom = r->top + (int)(w/aspectRatio) + window_adj_y;

		}
		return 0;

	case WM_KEYUP:
		OnKeyUp(wParam);
		return 0;

	case WM_TIMER:
		switch (wParam)
		{
		case 0:
			INPUT input;
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = 0x88; //Unassigned virtual keycode
			input.ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(1, &input, sizeof(INPUT));
			return 0;
		}
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void D3d10App::InitWnd()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = "D3d10AppWndClass";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, "RegisterClass failed", 0, 0);
		PostQuitMessage(0);
	}

	RECT r = {0, 0, clientWidth, clientHeight};
	AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);
	int width = r.right - r.left;
	int height = r.bottom - r.top;
	window_adj_x = width - clientWidth;
	window_adj_y = height - clientHeight;

	hWnd = CreateWindow("D3d10AppWndClass",
		caption.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		0,
		0,
		hInstance,
		this
		);

	if (!hWnd)
	{
		MessageBox(0, "CreateWindow failed", 0, 0);
		PostQuitMessage(0);
	}

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

void D3d10App::SetCaption(std::string cap)
{
	SetWindowText(hWnd, cap.c_str());
}

int D3d10App::InitD3d()
{
	HRESULT hr2 = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory));
	IDXGIAdapter *pAdapter;
	hr2 = pFactory->EnumAdapters(0, &pAdapter);
	IDXGIOutput *pOutput;
	hr2 = pAdapter->EnumOutputs(0, &pOutput);
	UINT num_modes = 0;
	DXGI_MODE_DESC mode;
	hr2 = pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &num_modes, 0);
	DXGI_MODE_DESC * pDescs = new DXGI_MODE_DESC[num_modes];
	hr2 = pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &num_modes, pDescs);

	UINT createDeviceFlags = 0;
	//#if defined(DEBUG) || defined(_DEBUG)  
	//	createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
	//#endif
	hr2 = D3D10CreateDevice(pAdapter, d3dDriverType, 0, createDeviceFlags, D3D10_SDK_VERSION, &d3dDev);

	IDXGIDevice1 * pDXGIDevice;
	hr2 = d3dDev->QueryInterface(__uuidof(IDXGIDevice1), (void **)&pDXGIDevice);

	pDXGIDevice->SetMaximumFrameLatency(1);

	DXGI_OUTPUT_DESC output_desc;
	pOutput->GetDesc(&output_desc);

	DXGI_MODE_DESC target_mode = { 0 };
	target_mode.Width = output_desc.DesktopCoordinates.right - output_desc.DesktopCoordinates.left;
	target_mode.Height = output_desc.DesktopCoordinates.bottom - output_desc.DesktopCoordinates.top;
	pOutput->FindClosestMatchingMode(&target_mode, &matching_mode, d3dDev);

	sd.BufferDesc = matching_mode;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = hWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;
	hr2 = pFactory->CreateSwapChain(d3dDev, &sd, &swapChain);
	OnResize();
	ReleaseCOM(pOutput);
	ReleaseCOM(pAdapter);
	ReleaseCOM(pFactory);

	delete[] pDescs;
	
	return 1;
}

void D3d10App::OnKeyUp(WPARAM wParam)
{
}

