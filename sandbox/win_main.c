// WindowsProject1.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include <stdio.h>
#include <assert.h>

#include "window_resources.h"
#include "d3d11_device.h"
#include "allocator.h"
#include "stretchy_buffer.h"
#include "render_resources.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[] = L"Sandbox";                  // The title bar text
WCHAR szWindowClass[] = L"SandboxClass";            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, struct Program *);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

struct Program
{
	D3D11Device *device;
	Allocator *allocator;
};

typedef struct Timer
{
	LARGE_INTEGER timer;
} Timer;

float delta_time(Timer *timer)
{
	// Pretty much copy-paste from msdn.
	LARGE_INTEGER counter, frequency;
	QueryPerformanceCounter(&counter);
	QueryPerformanceFrequency(&frequency);
	LONGLONG elapsed_time = counter.QuadPart - timer->timer.QuadPart;
	elapsed_time *= 1000000;
	elapsed_time /= frequency.QuadPart;
	timer->timer = counter;
	float dt = elapsed_time / 1000000.0f;
	return dt;
}

int not_quit = 1;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					 _In_opt_ HINSTANCE hPrevInstance,
					 _In_ LPWSTR    lpCmdLine,
					 _In_ int       nCmdShow)
{
	struct Program program = { .device = 0, .allocator = 0};
	char initial_allocator_buffer[256U];
	program.allocator = create_allocator(initial_allocator_buffer, sizeof(initial_allocator_buffer));
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.

	// Initialize global strings
	//LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	//LoadStringW(hInstance, IDC_WINDOWSPROJECT1, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow, &program))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT1));

	MSG msg;

	const unsigned stride = 6 * sizeof(float);
	float vertex_buffer[] = {
		0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f,
	};
	const unsigned n_vertices = sizeof(vertex_buffer) / stride;

	RenderResources *resources = d3d11_device_render_resources(program.device);
	Resource vb_resource = render_resources_create_vertex_buffer(resources, vertex_buffer, n_vertices, stride);

	UINT16 index_buffer[] = {
		0, 1, 2,
		2, 1, 3,
	};
	const unsigned n_indices = sizeof(index_buffer) / sizeof(index_buffer[0]);
	const unsigned index_stride = 16;
	Resource ib_resource = render_resources_create_index_buffer(resources, index_buffer, n_indices, index_stride);

	float raw_buffer[] = {
		0.5f, -0.1f, 1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
	};
	const unsigned raw_buffer_size = sizeof(raw_buffer);
	Resource rb_resource = render_resources_create_raw_buffer(resources, raw_buffer, raw_buffer_size);

	VertexElement elements[2] = {
		{.semantic = VS_POSITION,.type = VT_FLOAT3},
		{.semantic = VS_TEXCOORD,.type = VT_FLOAT3},
	};

	Resource vd_resource = render_resources_create_vertex_declaration(resources, elements, 2);

	const char vertex_shader_program[] =
		" \
		ByteAddressBuffer buffer : t0; \
		struct VS_INPUT \
		{ \
			float4 position : POSITION;\
			float3 color : TEXCOORD0;\
			uint vertex_id : SV_VertexID;\
			uint instance_id : SV_InstanceID;\
		};\
		\
		struct VS_OUTPUT \
		{ \
			float4 position : SV_POSITION;\
			float3 color : TEXCOORD0;\
		};\
		\
		VS_OUTPUT vs_main(VS_INPUT input) \
		{ \
			uint pos_byte_address = input.instance_id * 4 * 6;\
			uint col_byte_address = pos_byte_address + 4 * 2; \
			float2 position = asfloat(buffer.Load2(pos_byte_address)); \
			float3 color = asfloat(buffer.Load3(col_byte_address)); \
			VS_OUTPUT output; \
			output.position = input.position + float4(position, 0.0f, 0.0f); \
			output.color = color;\
			return output; \
		}; \
		\
		struct PS_OUTPUT \
		{ \
			float4 color : SV_TARGET0; \
		}; \
		\
		PS_OUTPUT ps_main(VS_OUTPUT input) \
		{ \
			PS_OUTPUT output; \
			output.color.rgb = input.color; \
			output.color.a = 1.0f; \
			return output; \
		}; \
		";
	Resource vs_resource = render_resources_create_shader_program(resources, SPT_VERTEX, vertex_shader_program, sizeof(vertex_shader_program));
	Resource ps_resource = render_resources_create_shader_program(resources, SPT_PIXEL, vertex_shader_program, sizeof(vertex_shader_program));

	Resource render_resources[] = {
		vb_resource,
		ib_resource,
		vd_resource,
		vs_resource,
		ps_resource,
		rb_resource,
	};
	const unsigned n_resources = sizeof(render_resources) / sizeof(render_resources[0]);

	RenderPackage *render_package = create_render_package(program.allocator, render_resources, n_resources, n_vertices, n_indices);
	render_package->n_instances = 2;

	Timer timer;
	float dt = 0.0f;
	float direction = 0.1f;
	delta_time(&timer);
	while (not_quit) {
		dt = delta_time(&timer);

		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (raw_buffer[0] < -1.0f || raw_buffer[0] > 1.0f) {
			direction *= -1.0f;
		}

		raw_buffer[0] += direction * dt;

		render_resource_raw_buffer_update(resources, rb_resource, raw_buffer, sizeof(raw_buffer));

		d3d11_device_clear(program.device);
		d3d11_device_render(program.device, render_package);
		d3d11_device_present(program.device);		
	}

	render_resources_destroy_raw_buffer(resources, rb_resource);
	render_resources_destroy_shader_program(resources, vs_resource);
	render_resources_destroy_shader_program(resources, ps_resource);
	render_resources_destroy_vertex_declaration(resources, vd_resource);
	render_resources_destroy_index_buffer(resources, ib_resource);
	render_resources_destroy_vertex_buffer(resources, vb_resource);

	destroy_render_package(render_package);

	d3d11_device_destroy(program.allocator, program.device);
	destroy_allocator(program.allocator);

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSPROJECT1));
	wcex.hCursor        = LoadCursor(0, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
	wcex.lpszClassName  = szWindowClass;
	wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, struct Program *program)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	  CW_USEDEFAULT, 0, 1280, 720, 0, 0, hInstance, 0);

   if (!hWnd)
   {
	  return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   d3d11_device_create(program->allocator, hWnd, &program->device);
   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);
			// Parse the menu selections:
			switch (wmId)
			{
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code that uses hdc here...
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		not_quit = 0;
		break;
	case WM_KEYUP:
		// Able to quit by pressing ESC.
		if (wParam == 27) {
			PostQuitMessage(0);
			not_quit = 0;
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
