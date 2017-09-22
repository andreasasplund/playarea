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
#include "stb_easy_font.h"

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

	RenderResources *resources = d3d11_device_render_resources(program.device);

	static const unsigned n_font_verts = 9999;
	static char buffer[1024 * 1024]; // ~500 chars
	UINT16 font_index_buffer[6 * 9999];
	for (unsigned i = 0, q = 0; i < 6 * 9999; i += 6, ++q) {
		const unsigned index = q * 6;
		font_index_buffer[index + 0] = q * 4 + 0;
		font_index_buffer[index + 1] = q * 4 + 1;
		font_index_buffer[index + 2] = q * 4 + 2;

		font_index_buffer[index + 3] = q * 4 + 0;
		font_index_buffer[index + 4] = q * 4 + 2;
		font_index_buffer[index + 5] = q * 4 + 3;
	}
	const unsigned n_font_indices = n_font_verts * 6;
	Resource font_vb_resource = render_resources_create_vertex_buffer(resources, NULL, n_font_verts, 4 * sizeof(float));
	Resource font_ib_resource = render_resources_create_index_buffer(resources, font_index_buffer, n_font_indices, sizeof(font_index_buffer[0]));

	const char font_shader_program[] =
		"\
		struct VS_INPUT \
		{ \
			float3 position : POSITION;\
			float4 color : COLOR;\
		};\
		\
		struct VS_OUTPUT \
		{ \
			float4 position : SV_POSITION;\
			float4 color : COLOR0;\
		};\
		\
		VS_OUTPUT vs_main(VS_INPUT input) \
		{ \
			VS_OUTPUT output; \
			float2 offset = float2(640, 360);\
			output.position = (float4(input.position - float3(offset, 0), 1.0f) / float4(offset, 1.0f, 1.0f));\
			output.position.y *= -1.0f;\
			output.color = input.color;\
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
			output.color = input.color;\
			return output; \
		}; \
		";
	Resource font_vs_resource = render_resources_create_shader_program(resources, SPT_VERTEX, font_shader_program, sizeof(font_shader_program));
	Resource font_ps_resource = render_resources_create_shader_program(resources, SPT_PIXEL, font_shader_program, sizeof(font_shader_program));

	VertexElement font_elements[] = {
		{ .semantic = VS_POSITION,.type = VT_FLOAT3 },
		{ .semantic = VS_COLOR,.type = VT_UBYTE4 },
	};
	Resource font_vd_resource = render_resources_create_vertex_declaration(resources, font_elements, sizeof(font_elements) / sizeof(font_elements[0]));

	Resource font_render_resources[] = {
		font_vb_resource,
		font_ib_resource,
		font_vd_resource,
		font_vs_resource,
		font_ps_resource,
	};
	const unsigned n_font_resources = sizeof(font_render_resources) / sizeof(font_render_resources[0]);

	RenderPackage *font_render_package = create_render_package(program.allocator, font_render_resources, n_font_resources, 0, 0);
	enum { n_instances = 10000 };
	enum { n_types = 10 };

	const unsigned stride = 3 * sizeof(float);
	float vertex_buffer[] = {
		0.0f, 0.01f, 0.0f,
		0.0f, -0.01f, 0.0f,
		0.01f, 0.01f, 0.0f,
		0.01f, -0.01f, 0.0f,
	};
	const unsigned n_vertices = sizeof(vertex_buffer) / stride;

	Resource vb_resource = render_resources_create_vertex_buffer(resources, vertex_buffer, n_vertices, stride);

	UINT16 index_buffer[] = {
		0, 1, 2,
		2, 1, 3,
	};
	const unsigned n_indices = sizeof(index_buffer) / sizeof(index_buffer[0]);
	const unsigned index_stride = sizeof(index_buffer[0]);
	Resource ib_resource = render_resources_create_index_buffer(resources, index_buffer, n_indices, index_stride);

	unsigned types_raw_buffer[n_instances];
	for (unsigned i = 0; i < n_instances; ++i) {
		types_raw_buffer[i] = i % n_types;
	}
	Resource types_rb_resource = render_resources_create_raw_buffer(resources, types_raw_buffer, sizeof(types_raw_buffer));

	float positions_raw_buffer[n_instances * 2];
	for (unsigned i = 0; i < n_instances; ++i) {
		unsigned index = 2 * i;
		int value;
		do {
			value = rand();
		} while (value == 0);
		positions_raw_buffer[index + 0] = 2.0f * (value / (float)RAND_MAX) - 1.0f;
		do {
			value = rand();
		} while (value == 0);
		positions_raw_buffer[index + 1] = 2.0f * value / (float)RAND_MAX - 1.0f;
	}
	positions_raw_buffer[0] = -0.5f;
	positions_raw_buffer[1] = 0.5f;
	Resource positions_rb_resource = render_resources_create_raw_buffer(resources, positions_raw_buffer, sizeof(positions_raw_buffer));

	float directions[n_instances];
	for (unsigned i = 0; i < n_instances; ++i) {
		directions[i] = 0.1f;
	}

	float colors_raw_buffer[n_types * 4] = {
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 0.2f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.2f, 1.0f,
		0.2f, 0.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 0.2f, 1.0f,
	};
	Resource colors_rb_resource = render_resources_create_raw_buffer(resources, colors_raw_buffer, sizeof(colors_raw_buffer));

	VertexElement elements[] = {
		{.semantic = VS_POSITION,.type = VT_FLOAT3},
	};
	const unsigned n_elements = sizeof(elements) / sizeof(elements[0]);

	Resource vd_resource = render_resources_create_vertex_declaration(resources, elements, n_elements);

	const char vertex_shader_program[] =
		" \
		ByteAddressBuffer positions_buffer : t0; \
		ByteAddressBuffer types_buffer : t1; \
		ByteAddressBuffer colors_buffer : t2; \
		struct VS_INPUT \
		{ \
			float4 position : POSITION;\
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
			uint pos_byte_address = input.instance_id * 4 * 2;\
			uint type_byte_address = input.instance_id * 4 * 1; \
			\
			float2 position = asfloat(positions_buffer.Load2(pos_byte_address)); \
			float type = types_buffer.Load(type_byte_address); \
			\
			uint col_byte_address = type * 4 * 4; \
			float4 color = asfloat(colors_buffer.Load4(col_byte_address)); \
			VS_OUTPUT output; \
			output.position = input.position + float4(position, 0.0f, 0.0f); \
			output.color = color.rgb;\
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
		positions_rb_resource,
		types_rb_resource,
		colors_rb_resource,
	};
	const unsigned n_resources = sizeof(render_resources) / sizeof(render_resources[0]);

	RenderPackage *render_package = create_render_package(program.allocator, render_resources, n_resources, n_vertices, n_indices);
	render_package->n_instances = n_instances;

	Timer timer;
	float dt = 0.0f;
	delta_time(&timer);
	float smoothed_dt = 0.0f;
	float smoothed_update_pos_time = 0.0f;
	while (not_quit) {
		dt = delta_time(&timer);

		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		Timer update_pos_timer;
		delta_time(&update_pos_timer);
		for (unsigned i = 0; i < n_instances; ++i) {
			const unsigned index = 2 * i;
			if (positions_raw_buffer[index] < -1.0f) {
				positions_raw_buffer[index] = -1.0f;
				directions[i] *= -1.0f;
			} else if (positions_raw_buffer[index] > 1.0f) {
				positions_raw_buffer[index] = 1.0f;
				directions[i] *= -1.0f;
			}

			positions_raw_buffer[index] += directions[i] * smoothed_dt;
		}
		float update_pos_time = delta_time(&update_pos_timer);
		smoothed_update_pos_time = smoothed_update_pos_time * 0.9f + update_pos_time * 0.1f;

		render_resource_raw_buffer_update(resources, positions_rb_resource, positions_raw_buffer, sizeof(positions_raw_buffer));

		int num_quads;
		unsigned char color[4] = { 255, 255, 255, 255 };
		unsigned char text_buffer[1024];
		smoothed_dt = smoothed_dt * 0.9f + dt * 0.1f;
		sprintf_s(text_buffer, 1024, "Instance count: %u\nUpdate loop time: %.2f\nUpdate pos time: %.10f", n_instances, smoothed_dt * 1000.0f, smoothed_update_pos_time* 1000.0f);
		num_quads = stb_easy_font_print(0, 0, text_buffer, color, buffer, sizeof(buffer));
		render_resource_vertex_buffer_update(resources, font_vb_resource, buffer, num_quads * 4 * sizeof(float) * 4);
		font_render_package->n_vertices = num_quads * 4;
		font_render_package->n_indices = num_quads * 6;

		d3d11_device_clear(program.device);
		d3d11_device_render(program.device, render_package);
		d3d11_device_render(program.device, font_render_package);
		d3d11_device_present(program.device);		
	}

	render_resources_destroy_raw_buffer(resources, positions_rb_resource);
	render_resources_destroy_raw_buffer(resources, types_rb_resource);
	render_resources_destroy_raw_buffer(resources, colors_rb_resource);
	render_resources_destroy_shader_program(resources, vs_resource);
	render_resources_destroy_shader_program(resources, ps_resource);
	render_resources_destroy_vertex_declaration(resources, vd_resource);
	render_resources_destroy_index_buffer(resources, ib_resource);
	render_resources_destroy_vertex_buffer(resources, vb_resource);

	destroy_render_package(render_package);

	render_resources_destroy_vertex_buffer(resources, font_vb_resource);
	render_resources_destroy_index_buffer(resources, font_ib_resource);
	render_resources_destroy_vertex_declaration(resources, font_vd_resource);
	render_resources_destroy_shader_program(resources, font_vs_resource);
	render_resources_destroy_shader_program(resources, font_ps_resource);
	destroy_render_package(font_render_package);

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
