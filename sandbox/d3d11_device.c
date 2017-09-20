#include "d3d11_device.h"

#define COBJMACROS
#include <dxgi.h>
#include <d3d11.h>
#include <assert.h>
#include <d3dcompiler.h>

#include "allocator.h"
#include "stretchy_buffer.h"
#include "render_resources.h"

struct D3D11Device
{
	IDXGIFactory1 *dxgi_factory;
	IDXGIAdapter *adapter;
	IDXGISwapChain *swap_chain;
	ID3D11Device *device;
	ID3D11DeviceContext *immediate_context;
	ID3D11Debug *debug_layer;

	Allocator *allocator;
	RenderResources *resources;

	// Default states
	ID3D11BlendState *blend_state;
	ID3D11DepthStencilState *depth_stencil_state;
	ID3D11RasterizerState *rasterizer_state;

	ID3D11Resource *swap_chain_texture;
	ID3D11RenderTargetView *swap_chain_rtv;
};

void d3d11_device_create(Allocator *allocator, HWND window, D3D11Device **d3d11_device)
{
	D3D11Device *device = *d3d11_device = allocator_realloc(allocator, NULL, sizeof(D3D11Device), 16);
	HRESULT hr = CreateDXGIFactory1(&IID_IDXGIFactory1, &device->dxgi_factory);
	if (FAILED(hr)) {
		assert(0);
		return;
	}

	if (FAILED(IDXGIFactory1_EnumAdapters(device->dxgi_factory, 0, &device->adapter) != DXGI_ERROR_NOT_FOUND)) {
		assert(0);
		return;
	}

	DXGI_ADAPTER_DESC desc;
	if (FAILED(IDXGIAdapter_GetDesc(device->adapter, &desc))) {
		assert(0);
		return;
	}

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
	hr = D3D11CreateDeviceAndSwapChain(device->adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, D3D11_CREATE_DEVICE_DEBUG, feature_levels, 5, D3D11_SDK_VERSION, &swap_chain_desc, &device->swap_chain, &device->device, &feature_level, &device->immediate_context);
	if (FAILED(hr)) {
		assert(0);
		return;
	}

	hr = ID3D11Device_QueryInterface(device->device, &IID_ID3D11Debug, &device->debug_layer);
	if (FAILED(hr)) {
		assert(0);
		return;
	}

	device->allocator = allocator;

	D3D11_BLEND_DESC blend_state_desc = { .AlphaToCoverageEnable = FALSE,.IndependentBlendEnable = FALSE,
		{ 0, 0, 0, 0, 0, 0, 0, 0xFU ,
		 0, 0, 0, 0, 0, 0, 0, 0xFU ,
		 0, 0, 0, 0, 0, 0, 0, 0xFU ,
		 0, 0, 0, 0, 0, 0, 0, 0xFU ,
		 0, 0, 0, 0, 0, 0, 0, 0xFU ,
		 0, 0, 0, 0, 0, 0, 0, 0xFU ,
		 0, 0, 0, 0, 0, 0, 0, 0xFU ,
		 0, 0, 0, 0, 0, 0, 0, 0xFU },
	};
	D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = { .DepthEnable = FALSE, .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL, .DepthFunc = D3D11_COMPARISON_ALWAYS };
	D3D11_RASTERIZER_DESC rasterizer_desc = { .FillMode = D3D11_FILL_SOLID,.CullMode = D3D11_CULL_NONE };

	hr = ID3D11Device_CreateBlendState(device->device, &blend_state_desc, &device->blend_state);
	if (FAILED(hr)) {
		assert(0);
		return;
	}
	hr = ID3D11Device_CreateDepthStencilState(device->device, &depth_stencil_desc, &device->depth_stencil_state);
	if (FAILED(hr)) {
		assert(0);
		return;
	}
	hr = ID3D11Device_CreateRasterizerState(device->device, &rasterizer_desc, &device->rasterizer_state);
	if (FAILED(hr)) {
		assert(0);
		return;
	}

	hr = IDXGISwapChain_GetBuffer(device->swap_chain, 0, &IID_ID3D11Resource, &device->swap_chain_texture);
	if (FAILED(hr)) {
		assert(0);
		return;
	}

	D3D11_RENDER_TARGET_VIEW_DESC rt_desc;
	rt_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rt_desc.Texture2DArray.MipSlice = 0;
	rt_desc.Texture2DArray.FirstArraySlice = 0;
	rt_desc.Texture2DArray.ArraySize = 1;
	rt_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	hr = ID3D11Device_CreateRenderTargetView(device->device, device->swap_chain_texture, &rt_desc, &device->swap_chain_rtv);
	if (FAILED(hr)) {
		assert(0);
		return;
	}

	render_resources_create(allocator, device->device, &device->resources);
}

void d3d11_device_destroy(Allocator *allocator, D3D11Device *device)
{
	render_resources_destroy(allocator, device->resources);

	ID3D11BlendState_Release(device->blend_state);
	ID3D11DepthStencilState_Release(device->depth_stencil_state);
	ID3D11RasterizerState_Release(device->rasterizer_state);

	ID3D11Resource_Release(device->swap_chain_texture);
	ID3D11RenderTargetView_Release(device->swap_chain_rtv);

	IDXGISwapChain_Release(device->swap_chain);
	ID3D11Debug_Release(device->debug_layer);
	ID3D11DeviceContext_Release(device->immediate_context);
	ID3D11Device_Release(device->device);
	IDXGIAdapter_Release(device->adapter);
	IDXGIFactory1_Release(device->dxgi_factory);

	allocator_realloc(allocator, device, 0, 0);
}

RenderResources *d3d11_device_render_resources(D3D11Device *device)
{
	return device->resources;
}

void d3d11_device_present(D3D11Device *device)
{
	HRESULT hr = IDXGISwapChain_Present(device->swap_chain, 0, 0);
	if (FAILED(hr)) {
		assert(0);
		return;
	}
}

void d3d11_device_clear(D3D11Device *device)
{
	static unsigned color_index = 0;
	color_index = color_index++ % 3;
	struct Color
	{
		float rgba[4];
	};
	struct Color colors[] = {
		{ 1.0, 0.0, 0.0, 1.0 },
		{ 0.0, 1.0, 0.0, 1.0 },
		{ 0.0, 0.0, 1.0, 1.0 },
		{ 0.0, 0.0, 0.0, 1.0 }
	};
	ID3D11DeviceContext_ClearRenderTargetView(device->immediate_context, device->swap_chain_rtv, colors[0].rgba);
}

void d3d11_device_render(D3D11Device *device, RenderPackage *render_package)
{
	Resource vs_res, vd_res;
	Buffer *vb = NULL;
	Buffer *ib = NULL;
	VertexDeclaration *vd = NULL;
	VertexShader *vs = NULL;
	PixelShader *ps = NULL;
	RawBuffer *rb = NULL;
	const unsigned n_resources = render_package->n_resources;
	for (unsigned i = 0; i < n_resources; ++i) {
		Resource resource = render_package->resources[i];
		const unsigned type = resource_type(resource);
		switch (type) {
		case RESOURCE_VERTEX_BUFFER:
			vb = render_resources_vertex_buffer(device->resources, resource);
			break;
		case RESOURCE_INDEX_BUFFER:
			ib = render_resources_index_buffer(device->resources, resource);
			break;
		case RESOURCE_VERTEX_DECLARATION:
			vd = render_resources_vertex_declaration(device->resources, resource);
			vd_res = resource;
			break;
		case RESOURCE_VERTEX_SHADER:
			vs = render_resources_vertex_shader(device->resources, resource);
			vs_res = resource;
			break;
		case RESOURCE_PIXEL_SHADER:
			ps = render_resources_pixel_shader(device->resources, resource);
			break;
		case RESOURCE_RAW_BUFFER:
			rb = render_resources_raw_buffer(device->resources, resource);
			break;
		}
	}

	InputLayout *in_layout = render_resources_input_layout(device->resources, vs_res, vd_res);

	ID3D11DeviceContext_IASetInputLayout(device->immediate_context, in_layout->input_layout);
	ID3D11DeviceContext_OMSetRenderTargets(device->immediate_context, 1, &device->swap_chain_rtv, NULL);
	ID3D11DeviceContext_IASetPrimitiveTopology(device->immediate_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	D3D11_VIEWPORT viewport = {
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = 1280,
		.Height = 720,
		.MinDepth = 0,
		.MaxDepth = 1,
	};
	ID3D11DeviceContext_RSSetViewports(device->immediate_context, 1, &viewport);
	D3D11_RECT rect = { 0, 0, 1280, 720 };
	ID3D11DeviceContext_RSSetScissorRects(device->immediate_context, 1, &rect);

	ID3D11DeviceContext_VSSetShader(device->immediate_context, vs->shader, NULL, 0);
	ID3D11DeviceContext_PSSetShader(device->immediate_context, ps->shader, NULL, 0);

	if (rb)
		ID3D11DeviceContext_VSSetShaderResources(device->immediate_context, 0, 1, &rb->srv);
	else
		ID3D11DeviceContext_VSSetShaderResources(device->immediate_context, 0, 0, NULL);

	UINT stride = vb->stride;
	UINT offset = 0;
	ID3D11DeviceContext_IASetVertexBuffers(device->immediate_context, 0, 1, &vb->buffer, &stride, &offset);
	DXGI_FORMAT ib_format = ib ? (ib->stride == 16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT) : DXGI_FORMAT_R16_UINT;
	ID3D11Buffer *ib_buffer = ib ? ib->buffer : NULL;
	ID3D11DeviceContext_IASetIndexBuffer(device->immediate_context, ib_buffer, ib_format, 0);

	ID3D11DeviceContext_OMSetDepthStencilState(device->immediate_context, device->depth_stencil_state, 0);
	ID3D11DeviceContext_OMSetBlendState(device->immediate_context, device->blend_state, 0, 0xFFFFFFFFU);
	ID3D11DeviceContext_RSSetState(device->immediate_context, device->rasterizer_state);

	if (ib) {
		ID3D11DeviceContext_DrawIndexedInstanced(device->immediate_context, render_package->n_indices, render_package->n_instances, 0, 0, 0);
	} else {
		ID3D11DeviceContext_DrawInstanced(device->immediate_context, render_package->n_vertices, render_package->n_instances, 0, 0);
	}
}