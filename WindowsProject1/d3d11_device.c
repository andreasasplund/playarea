#include "d3d11_device.h"

#include <dxgi.h>
#include <d3d11.h>
#include <Unknwnbase.h>

#include "allocator.h"

struct D3D11Device
{
	IDXGIFactory1 *dxgi_factory;
	IDXGIAdapter *adapter;
	IDXGISwapChain *swap_chain;
	ID3D11Device *device;
	ID3D11DeviceContext *immediate_context;
	ID3D11Debug *debug_layer;
};

int initialize_d3d11_device(struct Allocator *allocator, HWND window, struct D3D11Device **d3d11_device)
{
	struct D3D11Device *device = *d3d11_device = allocator_allocate(allocator, sizeof(struct D3D11Device), 16);
	HRESULT hr = CreateDXGIFactory1(&IID_IDXGIFactory1, &device->dxgi_factory);
	if (FAILED(hr))
		return -1;

	if (FAILED(IDXGIFactory1_EnumAdapters(device->dxgi_factory, 0, &device->adapter) != DXGI_ERROR_NOT_FOUND))
		return -1;

	DXGI_ADAPTER_DESC desc;
	if (FAILED(IDXGIAdapter_GetDesc(device->adapter, &desc)))
		return -1;

	DXGI_SWAP_CHAIN_DESC swap_chain_desc = {
		.BufferCount = 2,
		.BufferDesc.Width = 1280,
		.BufferDesc.Height = 720,
		.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.BufferDesc.RefreshRate.Numerator = 0,
		.BufferDesc.RefreshRate.Denominator = 0,
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT,
		.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
		.OutputWindow = window,
		.SampleDesc.Count = 1,
		.SampleDesc.Quality = 0,
		.SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
		.Windowed = 1,
	};

	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_1;
	D3D_FEATURE_LEVEL feature_levels[5] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,	D3D_FEATURE_LEVEL_9_3 };
	hr = D3D11CreateDevice(device->adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, D3D11_CREATE_DEVICE_DEBUG, feature_levels, 5, D3D11_SDK_VERSION, &device->device, &feature_level, &device->immediate_context);
	//hr = D3D11CreateDeviceAndSwapChain(device->adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, D3D11_CREATE_DEVICE_DEBUG, feature_levels, 5, D3D11_SDK_VERSION, &swap_chain_desc, &device->swap_chain, &device->device, &feature_level, &device->immediate_context);
	if (FAILED(hr))
		return -1;

	hr = ID3D11Device_QueryInterface(device->device, &IID_ID3D11Debug, &device->debug_layer);
	if (FAILED(hr))
		return -1;

	IUnknown *unknown_device;
	hr = ID3D11Device_QueryInterface(device->device, &IID_IUnknown, &unknown_device);
	if (FAILED(hr))
		return -1;

	hr = IDXGIFactory1_CreateSwapChain(device->dxgi_factory, unknown_device, &swap_chain_desc, &device->swap_chain);
	if (FAILED(hr))
		return -1;

	IUnknown_Release(unknown_device);

	return 0;
}

static ID3D11Resource *texture = 0;
static ID3D11RenderTargetView *rt_view = 0;

void shutdown_d3d11_device(struct Allocator *allocator, struct D3D11Device *d3d11_device)
{
	IDXGISwapChain_Release(d3d11_device->swap_chain);
	ID3D11Debug_Release(d3d11_device->debug_layer);
	ID3D11DeviceContext_Release(d3d11_device->immediate_context);
	ID3D11Device_Release(d3d11_device->device);
	IDXGIAdapter_Release(d3d11_device->adapter);
	IDXGIFactory1_Release(d3d11_device->dxgi_factory);

	ID3D11Resource_Release(texture);
	ID3D11RenderTargetView_Release(rt_view);

	allocator_free(allocator, d3d11_device);
}

int d3d11_device_update(struct D3D11Device *device)
{
	if (texture == 0) {
		HRESULT hr = IDXGISwapChain_GetBuffer(device->swap_chain, 0, &IID_ID3D11Resource, &texture);
		if (FAILED(hr))
			return -1;

		D3D11_RENDER_TARGET_VIEW_DESC rt_desc;
		rt_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rt_desc.Texture2DArray.MipSlice = 0;
		rt_desc.Texture2DArray.FirstArraySlice = 0;
		rt_desc.Texture2DArray.ArraySize = 1;
		rt_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;



		hr = ID3D11Device_CreateRenderTargetView(device->device, texture, &rt_desc, &rt_view);
		if (FAILED(hr))
			return -1;
	}

	static unsigned color_index = 0;
	color_index = color_index++ % 3;
	struct Color {
		float rgba[4];
	};
	struct Color colors[3] = {
		{ 1.0, 0.0, 0.0, 1.0 },
		{ 0.0, 1.0, 0.0, 1.0 },
		{ 0.0, 0.0, 1.0, 1.0 },
	};
	ID3D11DeviceContext_ClearRenderTargetView(device->immediate_context, rt_view, colors[color_index].rgba);

	auto hr = IDXGISwapChain_Present(device->swap_chain, 0, 0);
	if (FAILED(hr))
		return -1;

	return 0;
}