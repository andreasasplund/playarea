#include "d3d11_device.h"

#include <dxgi.h>
#include <d3d11.h>
#include <assert.h>

#include "allocator.h"
#include "stretchy_buffer.h"

typedef struct VertexBuffer
{
	ID3D11Buffer *buffer;
} VertexBuffer;

typedef struct VertexDeclaration
{
	D3D11_INPUT_ELEMENT_DESC *elements;
} VertexDeclaration;

typedef struct Resources
{
	VertexBuffer *vertex_buffers;
	unsigned *free_vertex_buffers;

	VertexDeclaration *vertex_declarations;
	unsigned *free_vertex_declarations;
} Resources_t;

typedef struct RenderPackage
{
	Allocator *allocator;
	Resource *resources;
	unsigned n_resources;
} RenderPackage;

struct D3D11Device
{
	IDXGIFactory1 *dxgi_factory;
	IDXGIAdapter *adapter;
	IDXGISwapChain *swap_chain;
	ID3D11Device *device;
	ID3D11DeviceContext *immediate_context;
	ID3D11Debug *debug_layer;

	struct Allocator *allocator;
	Resources_t resources;
};

Resource allocate_vertex_buffer_handle(struct D3D11Device *device);
void release_vertex_buffer_handle(struct D3D11Device *device, Resource resource);
Resource allocate_vertex_declaration_handle(struct D3D11Device *device);
void release_vertex_declaration_handle(struct D3D11Device *device, Resource resource);

int initialize_d3d11_device(struct Allocator *allocator, HWND window, struct D3D11Device **d3d11_device)
{
	struct D3D11Device *device = *d3d11_device = allocator_realloc(allocator, NULL, sizeof(struct D3D11Device), 16);
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
	hr = D3D11CreateDeviceAndSwapChain(device->adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, D3D11_CREATE_DEVICE_DEBUG, feature_levels, 5, D3D11_SDK_VERSION, &swap_chain_desc, &device->swap_chain, &device->device, &feature_level, &device->immediate_context);
	if (FAILED(hr))
		return -1;

	hr = ID3D11Device_QueryInterface(device->device, &IID_ID3D11Debug, &device->debug_layer);
	if (FAILED(hr))
		return -1;


	device->allocator = allocator;
	sb_create(allocator, device->resources.vertex_buffers, 10);
	sb_create(allocator, device->resources.free_vertex_buffers, 10);
	Resource first_vb = allocate_vertex_buffer_handle(device);
	assert(resource_handle(first_vb) == 0);
	(void)first_vb;

	sb_create(allocator, device->resources.vertex_declarations, 10);
	sb_create(allocator, device->resources.free_vertex_declarations, 10);
	Resource first_vd = allocate_vertex_declaration_handle(device);
	assert(resource_handle(first_vd) == 0);
	(void)first_vd;

	return 0;
}

static ID3D11Resource *texture = 0;
static ID3D11RenderTargetView *rt_view = 0;

void shutdown_d3d11_device(struct Allocator *allocator, struct D3D11Device *d3d11_device)
{
	release_vertex_buffer_handle(d3d11_device, resource_encode_handle_type(0, RESOURCE_VERTEX_BUFFER));
	release_vertex_declaration_handle(d3d11_device, resource_encode_handle_type(0, RESOURCE_VERTEX_DECLARATION));

	IDXGISwapChain_Release(d3d11_device->swap_chain);
	ID3D11Debug_Release(d3d11_device->debug_layer);
	ID3D11DeviceContext_Release(d3d11_device->immediate_context);
	ID3D11Device_Release(d3d11_device->device);
	IDXGIAdapter_Release(d3d11_device->adapter);
	IDXGIFactory1_Release(d3d11_device->dxgi_factory);

	ID3D11Resource_Release(texture);
	ID3D11RenderTargetView_Release(rt_view);

	sb_free(d3d11_device->resources.vertex_buffers);
	sb_free(d3d11_device->resources.free_vertex_buffers);

	sb_free(d3d11_device->resources.vertex_declarations);
	sb_free(d3d11_device->resources.free_vertex_declarations);

	allocator_realloc(allocator, d3d11_device, 0, 0);
}

int d3d11_device_present(struct D3D11Device *device)
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

Resource allocate_vertex_buffer_handle(struct D3D11Device *device)
{
	if (sb_count(device->resources.free_vertex_buffers)) {
		unsigned h = sb_last(device->resources.free_vertex_buffers);
		sb_pop(device->resources.free_vertex_buffers);
		return resource_encode_handle_type(h, RESOURCE_VERTEX_BUFFER);
	}

	unsigned h = sb_count(device->resources.vertex_buffers);
	VertexBuffer vertex_buffer = { .buffer = NULL };
	sb_push(device->resources.vertex_buffers, vertex_buffer);

	return resource_encode_handle_type(h, RESOURCE_VERTEX_BUFFER);
}

void release_vertex_buffer_handle(struct D3D11Device *device, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_BUFFER);
	unsigned h = resource_handle(resource);
	sb_push(device->resources.free_vertex_buffers, h);
}

Resource allocate_vertex_declaration_handle(struct D3D11Device *device)
{
	if (sb_count(device->resources.free_vertex_declarations)) {
		unsigned h = sb_last(device->resources.free_vertex_declarations);
		sb_pop(device->resources.free_vertex_declarations);
		return resource_encode_handle_type(h, RESOURCE_VERTEX_DECLARATION);
	}

	unsigned h = sb_count(device->resources.vertex_declarations);
	VertexDeclaration vertex_declaration = { .elements = NULL };
	sb_push(device->resources.vertex_declarations, vertex_declaration);

	return resource_encode_handle_type(h, RESOURCE_VERTEX_DECLARATION);
}

void release_vertex_declaration_handle(struct D3D11Device *device, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_DECLARATION);
	unsigned h = resource_handle(resource);
	sb_push(device->resources.free_vertex_declarations, h);
}

VertexBuffer *vertex_buffer(struct D3D11Device *device, Resource resource)
{
	return &device->resources.vertex_buffers[resource_handle(resource)];
}

Resource create_vertex_buffer(struct D3D11Device *device, void *buffer, unsigned vertices, unsigned stride)
{
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = vertices * stride;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA sub_desc;
	sub_desc.SysMemPitch = 0;
	sub_desc.SysMemSlicePitch = 0;
	sub_desc.pSysMem = buffer;

	Resources_t *resources = &device->resources;
	Resource vb_res = allocate_vertex_buffer_handle(device);

	VertexBuffer *vb = vertex_buffer(device, vb_res);

	HRESULT hr = ID3D11Device_CreateBuffer(device->device, &desc, buffer ? &sub_desc : 0, &vb->buffer);
	assert(SUCCEEDED(hr));

	return vb_res;
}

void destroy_vertex_buffer(struct D3D11Device *device, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_BUFFER);

	VertexBuffer *vb = vertex_buffer(device, resource);
	ID3D11Buffer_Release(vb->buffer);

	release_vertex_buffer_handle(device, resource);
}

VertexDeclaration *vertex_declaration(struct D3D11Device *device, Resource resource)
{
	return &device->resources.vertex_declarations[resource_handle(resource)];
}

Resource create_vertex_declaration(struct D3D11Device *device, VertexElement_t *vertex_elements, unsigned n_vertex_elements)
{
	Resources_t *resources = &device->resources;
	Resource vd_res = allocate_vertex_declaration_handle(device);

	VertexDeclaration *vd = vertex_declaration(device, vd_res);

	sb_create(device->allocator, vd->elements, n_vertex_elements);

	static const char* semantics[] = { "POSITION", "COLOR" };
	static const DXGI_FORMAT formats[] = { DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT };
	static unsigned type_size[] = {
		3 * sizeof(float),
		4 * sizeof(float),
	};

	unsigned offset = 0;

	for (unsigned i = 0; i < n_vertex_elements; ++i) {
		D3D11_INPUT_ELEMENT_DESC element = {
			.SemanticName = semantics[vertex_elements[i].semantic],
			.SemanticIndex = 0,
			.Format = formats[vertex_elements[i].type],
			.InputSlot = 0,
			.AlignedByteOffset = offset,
			.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0,
		};
		sb_push(vd->elements, element);

		offset += type_size[vertex_elements[i].type];
	}

	return vd_res;
}

void destroy_vertex_declaration(struct D3D11Device *device, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_DECLARATION);

	VertexDeclaration *vd = vertex_declaration(device, resource);
	sb_free(vd->elements);

	release_vertex_declaration_handle(device, resource);
}

RenderPackage *create_render_package(Allocator *allocator, const Resource *resources, unsigned n_resources)
{
	RenderPackage *package = (RenderPackage*)allocator_realloc(allocator, NULL, sizeof(RenderPackage) + sizeof(Resource) * n_resources, 16);
	package->allocator = allocator;
	package->n_resources = n_resources;
	package->resources = (Resource*)((uintptr_t)package + sizeof(RenderPackage));
	memcpy(package->resources, resources, sizeof(Resource) * n_resources);

	return package;
}

void destroy_render_package(RenderPackage *render_package)
{
	allocator_realloc(render_package->allocator, render_package, 0, 0);
}

void d3d11_device_render(D3D11Device *device, RenderPackage *render_package)
{
	VertexBuffer *vb = NULL;
	VertexDeclaration *vd = NULL;
	const unsigned n_resources = render_package->n_resources;
	for (unsigned i = 0; i < n_resources; ++i) {
		Resource resource = render_package->resources[i];
		const unsigned type = resource_type(resource);
		switch (type) {
		case RESOURCE_VERTEX_BUFFER:
			vb = vertex_buffer(device, resource);
			break;
		case RESOURCE_VERTEX_DECLARATION:
			vd = vertex_declaration(device, resource);
			break;
		}
	}

	ID3D11DeviceContext_IASetVertexBuffers(device->immediate_context, 0, 1, &vb->buffer, 0, 0);
}