// WindowsProject1.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include <stdio.h>
#include <assert.h>

#include "WindowsProject1.h"
#include "d3d11_device.h"
#include "allocator.h"
#include "stretchy_buffer.h"
#include "resources.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

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
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_WINDOWSPROJECT1, szWindowClass, MAX_LOADSTRING);
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
		-0.9f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
		-0.9f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
		-0.1f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
		-0.1f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f,
	};
	const unsigned n_vertices = sizeof(vertex_buffer) / stride;

	Resources *resources = d3d11_resources(program.device);
	Resource vb_resource = create_vertex_buffer(resources, vertex_buffer, n_vertices, stride);

	float vertex_buffer2[] = {
		0.1f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.1f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
		0.9f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f,

		0.9f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
		0.1f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
		0.9f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f,
	};
	const unsigned n_vertices2 = sizeof(vertex_buffer2) / stride;

	Resource vb2_resource = create_vertex_buffer(resources, vertex_buffer2, n_vertices2, stride);

	UINT16 index_buffer[] = {
		0, 1, 2,
		2, 1, 3,
	};
	const unsigned n_indices = sizeof(index_buffer) / sizeof(index_buffer[0]);
	const unsigned index_stride = 16;
	Resource ib_resource = create_index_buffer(resources, index_buffer, n_indices, index_stride);

	VertexElement elements[2] = {
		{.semantic = VS_POSITION,.type = VT_FLOAT3},
		{.semantic = VS_TEXCOORD,.type = VT_FLOAT3},
	};

	Resource vd_resource = create_vertex_declaration(resources, elements, 2);

	const char vertex_shader_program[] =
		" \
		struct VS_INPUT \
		{ \
			float4 position : POSITION;\
			float3 color : TEXCOORD0;\
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
			VS_OUTPUT output; \
			output.position = input.position; \
			output.color = input.color; \
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
	Resource vs_resource = create_shader_program(resources, SPT_VERTEX, vertex_shader_program, sizeof(vertex_shader_program));
	Resource ps_resource = create_shader_program(resources, SPT_PIXEL, vertex_shader_program, sizeof(vertex_shader_program));

	const char vertex_shader_program2[] =
		" \
		struct VS_INPUT \
		{ \
			float4 position : POSITION;\
		};\
		\
		struct VS_OUTPUT \
		{ \
			float4 position : SV_POSITION;\
		};\
		\
		VS_OUTPUT vs_main(VS_INPUT input) \
		{ \
			VS_OUTPUT output; \
			output.position = input.position; \
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
			output.color = float4(1.0f, 0.0f, 1.0f, 1.0f); \
			return output; \
		}; \
		";
	Resource vs_resource2 = create_shader_program(resources, SPT_VERTEX, vertex_shader_program2, sizeof(vertex_shader_program2));
	Resource ps_resource2 = create_shader_program(resources, SPT_PIXEL, vertex_shader_program2, sizeof(vertex_shader_program2));

	Resource render_resources[] = {
		vb_resource,
		ib_resource,
		vd_resource,
		vs_resource,
		ps_resource,
	};
	const unsigned n_resources = sizeof(render_resources) / sizeof(render_resources[0]);

	RenderPackage *render_package = create_render_package(program.allocator, render_resources, n_resources, n_vertices, n_indices);

	render_resources[0] = vb2_resource;
	render_resources[1] = resource_encode_handle_type(0, 0);
	render_resources[3] = vs_resource2;
	render_resources[4] = ps_resource2;
	RenderPackage *render_package2 = create_render_package(program.allocator, render_resources, n_resources, n_vertices2, n_indices);

	while (not_quit) {
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		d3d11_device_clear(program.device);
		d3d11_device_render(program.device, render_package);
		d3d11_device_render(program.device, render_package2);
		d3d11_device_present(program.device);
	}

	destroy_shader_program(resources, vs_resource);
	destroy_shader_program(resources, ps_resource);
	destroy_vertex_declaration(resources, vd_resource);
	destroy_index_buffer(resources, ib_resource);
	destroy_vertex_buffer(resources, vb_resource);

	destroy_render_package(render_package);
	destroy_render_package(render_package2);

	shutdown_d3d11_device(program.allocator, program.device);
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

   if (initialize_d3d11_device(program->allocator, hWnd, &program->device) < 0)
	   return FALSE;

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
