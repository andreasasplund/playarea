#include "d3d11_device.h"

#include <dxgi.h>
#include <d3d11.h>
#include <assert.h>
#include <d3dcompiler.h>

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

typedef struct VertexShader
{
	ID3D11VertexShader *shader;
	ID3DBlob *bytecode;
} VertexShader;

typedef struct PixelShader
{
	ID3D11PixelShader *shader;
} PixelShader;

typedef struct InputLayout
{
	ID3D11InputLayout *input_layout;
} InputLayout;

typedef struct Resources
{
	VertexBuffer *vertex_buffers;
	unsigned *free_vertex_buffers;

	VertexDeclaration *vertex_declarations;
	unsigned *free_vertex_declarations;

	VertexShader *vertex_shaders;
	unsigned *free_vertex_shaders;

	PixelShader *pixel_shaders;
	unsigned *free_pixel_shaders;

	InputLayout *input_layouts;
	UINT64 *input_layout_hashes;

	// Default states
	ID3D11BlendState *blend_state;
	ID3D11DepthStencilState *depth_stencil_state;
	ID3D11RasterizerState *rasterizer_state;

	ID3D11Resource *swap_chain_texture;
	ID3D11RenderTargetView *swap_chain_rtv;
} Resources_t;

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

typedef struct RenderPackage
{
	Allocator *allocator;
	Resource *resources;
	unsigned n_resources;
} RenderPackage;

Resource allocate_vertex_buffer_handle(D3D11Device *device)
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

void release_vertex_buffer_handle(D3D11Device *device, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_BUFFER);
	unsigned h = resource_handle(resource);
	sb_push(device->resources.free_vertex_buffers, h);
}

Resource allocate_vertex_declaration_handle(D3D11Device *device)
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

void release_vertex_declaration_handle(D3D11Device *device, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_DECLARATION);
	unsigned h = resource_handle(resource);
	sb_push(device->resources.free_vertex_declarations, h);
}

Resource allocate_vertex_shader_handle(D3D11Device *device)
{
	if (sb_count(device->resources.free_vertex_shaders)) {
		unsigned h = sb_last(device->resources.free_vertex_shaders);
		sb_pop(device->resources.free_vertex_shaders);
		return resource_encode_handle_type(h, RESOURCE_VERTEX_SHADER);
	}

	unsigned h = sb_count(device->resources.vertex_shaders);
	VertexShader vertex_shader = { .shader = NULL };
	sb_push(device->resources.vertex_shaders, vertex_shader);

	return resource_encode_handle_type(h, RESOURCE_VERTEX_SHADER);
}

void release_vertex_shader_handle(D3D11Device *device, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_SHADER);
	unsigned h = resource_handle(resource);
	sb_push(device->resources.free_vertex_shaders, h);
}

Resource allocate_pixel_shader_handle(D3D11Device *device)
{
	if (sb_count(device->resources.free_pixel_shaders)) {
		unsigned h = sb_last(device->resources.free_pixel_shaders);
		sb_pop(device->resources.free_pixel_shaders);
		return resource_encode_handle_type(h, RESOURCE_PIXEL_SHADER);
	}

	unsigned h = sb_count(device->resources.pixel_shaders);
	PixelShader pixel_shader = { .shader = NULL };
	sb_push(device->resources.pixel_shaders, pixel_shader);

	return resource_encode_handle_type(h, RESOURCE_PIXEL_SHADER);
}

void release_pixel_shader_handle(D3D11Device *device, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_PIXEL_SHADER);
	unsigned h = resource_handle(resource);
	sb_push(device->resources.free_pixel_shaders, h);
}

int initialize_d3d11_device(struct Allocator *allocator, HWND window, D3D11Device **d3d11_device)
{
	D3D11Device *device = *d3d11_device = allocator_realloc(allocator, NULL, sizeof(D3D11Device), 16);
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
	{
		sb_create(allocator, device->resources.vertex_buffers, 10);
		sb_create(allocator, device->resources.free_vertex_buffers, 10);
		Resource first_vb = allocate_vertex_buffer_handle(device);
		assert(resource_handle(first_vb) == 0);
		(void)first_vb;
	}

	{
		sb_create(allocator, device->resources.vertex_declarations, 10);
		sb_create(allocator, device->resources.free_vertex_declarations, 10);
		Resource first_vd = allocate_vertex_declaration_handle(device);
		assert(resource_handle(first_vd) == 0);
		(void)first_vd;
	}

	{
		sb_create(allocator, device->resources.vertex_shaders, 10);
		sb_create(allocator, device->resources.free_vertex_shaders, 10);
		Resource first_vs = allocate_vertex_shader_handle(device);
		assert(resource_handle(first_vs) == 0);
		(void)first_vs;
	}

	{
		sb_create(allocator, device->resources.pixel_shaders, 10);
		sb_create(allocator, device->resources.free_pixel_shaders, 10);
		Resource first_ps = allocate_pixel_shader_handle(device);
		assert(resource_handle(first_ps) == 0);
		(void)first_ps;
	}
	{
		sb_create(allocator, device->resources.input_layouts, 10);
		sb_create(allocator, device->resources.input_layout_hashes, 10);
	}

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

	ID3D11Device_CreateBlendState(device->device, &blend_state_desc, &device->resources.blend_state);
	ID3D11Device_CreateDepthStencilState(device->device, &depth_stencil_desc, &device->resources.depth_stencil_state);
	ID3D11Device_CreateRasterizerState(device->device, &rasterizer_desc, &device->resources.rasterizer_state);

	hr = IDXGISwapChain_GetBuffer(device->swap_chain, 0, &IID_ID3D11Resource, &device->resources.swap_chain_texture);
	if (FAILED(hr))
		return -1;

	D3D11_RENDER_TARGET_VIEW_DESC rt_desc;
	rt_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rt_desc.Texture2DArray.MipSlice = 0;
	rt_desc.Texture2DArray.FirstArraySlice = 0;
	rt_desc.Texture2DArray.ArraySize = 1;
	rt_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	hr = ID3D11Device_CreateRenderTargetView(device->device, device->resources.swap_chain_texture, &rt_desc, &device->resources.swap_chain_rtv);
	if (FAILED(hr))
		return -1;

	return 0;
}

void shutdown_d3d11_device(struct Allocator *allocator, D3D11Device *device)
{
	release_vertex_buffer_handle(device, resource_encode_handle_type(0, RESOURCE_VERTEX_BUFFER));
	release_vertex_declaration_handle(device, resource_encode_handle_type(0, RESOURCE_VERTEX_DECLARATION));
	release_vertex_shader_handle(device, resource_encode_handle_type(0, RESOURCE_VERTEX_SHADER));
	release_pixel_shader_handle(device, resource_encode_handle_type(0, RESOURCE_PIXEL_SHADER));

	const unsigned n_input_layouts = sb_count(device->resources.input_layouts);
	for (unsigned i = 0; i < n_input_layouts; ++i) {
		ID3D11InputLayout_Release(device->resources.input_layouts[i].input_layout);
	}

	ID3D11BlendState_Release(device->resources.blend_state);
	ID3D11DepthStencilState_Release(device->resources.depth_stencil_state);
	ID3D11RasterizerState_Release(device->resources.rasterizer_state);

	ID3D11Resource_Release(device->resources.swap_chain_texture);
	ID3D11RenderTargetView_Release(device->resources.swap_chain_rtv);

	IDXGISwapChain_Release(device->swap_chain);
	ID3D11Debug_Release(device->debug_layer);
	ID3D11DeviceContext_Release(device->immediate_context);
	ID3D11Device_Release(device->device);
	IDXGIAdapter_Release(device->adapter);
	IDXGIFactory1_Release(device->dxgi_factory);

	sb_free(device->resources.vertex_buffers);
	sb_free(device->resources.free_vertex_buffers);
	sb_free(device->resources.vertex_declarations);
	sb_free(device->resources.free_vertex_declarations);
	sb_free(device->resources.vertex_shaders);
	sb_free(device->resources.free_vertex_shaders);
	sb_free(device->resources.pixel_shaders);
	sb_free(device->resources.free_pixel_shaders);
	sb_free(device->resources.input_layouts);
	sb_free(device->resources.input_layout_hashes);

	allocator_realloc(allocator, device, 0, 0);
}

int d3d11_device_present(D3D11Device *device)
{
	auto hr = IDXGISwapChain_Present(device->swap_chain, 0, 0);
	if (FAILED(hr))
		return -1;

	return 0;
}

VertexBuffer *vertex_buffer(D3D11Device *device, Resource resource)
{
	return &device->resources.vertex_buffers[resource_handle(resource)];
}

Resource create_vertex_buffer(D3D11Device *device, void *buffer, unsigned vertices, unsigned stride)
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

void destroy_vertex_buffer(D3D11Device *device, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_BUFFER);

	VertexBuffer *vb = vertex_buffer(device, resource);
	ID3D11Buffer_Release(vb->buffer);

	release_vertex_buffer_handle(device, resource);
}

VertexDeclaration *vertex_declaration(D3D11Device *device, Resource resource)
{
	return &device->resources.vertex_declarations[resource_handle(resource)];
}

Resource create_vertex_declaration(D3D11Device *device, VertexElement_t *vertex_elements, unsigned n_vertex_elements)
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

void destroy_vertex_declaration(D3D11Device *device, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_DECLARATION);

	VertexDeclaration *vd = vertex_declaration(device, resource);
	sb_free(vd->elements);

	release_vertex_declaration_handle(device, resource);
}

VertexShader *vertex_shader(D3D11Device *device, Resource resource)
{
	return &device->resources.vertex_shaders[resource_handle(resource)];
}

PixelShader *pixel_shader(D3D11Device *device, Resource resource)
{
	return &device->resources.pixel_shaders[resource_handle(resource)];
}

Resource create_shader_program(D3D11Device *device, unsigned shader_program_type, const char *program, unsigned program_length)
{
	static const char *entry[] = { "vs_main", "ps_main" };
	static const char *target[] = { "vs_5_0", "ps_5_0" };
	ID3DBlob *shader_program, *error_blob;
	HRESULT hr = D3DCompile(program, program_length, "empty", NULL, NULL, entry[shader_program_type], target[shader_program_type], 0, 0, &shader_program, &error_blob);
	if (FAILED(hr)) {
		const char *error_str = (const char*)(ID3D10Blob_GetBufferPointer(error_blob));
		assert(0);
	}

	switch (shader_program_type) {
		case SPT_VERTEX:
		{
			Resource shader = allocate_vertex_shader_handle(device);
			VertexShader *vs = vertex_shader(device, shader);
			vs->bytecode = shader_program;
			hr = ID3D11Device_CreateVertexShader(device->device, ID3D10Blob_GetBufferPointer(shader_program), ID3D10Blob_GetBufferSize(shader_program), NULL, &vs->shader);
			assert(SUCCEEDED(hr));
			return shader;
		}
		break;
		case SPT_PIXEL:
		{
			Resource shader = allocate_pixel_shader_handle(device);
			PixelShader *ps = pixel_shader(device, shader);
			hr = ID3D11Device_CreatePixelShader(device->device, ID3D10Blob_GetBufferPointer(shader_program), ID3D10Blob_GetBufferSize(shader_program), NULL, &ps->shader);
			assert(SUCCEEDED(hr));
			return shader;
		}
		break;
		default:
		{
			assert(0);
		}
			break;
	}

	return resource_encode_handle_type(0, 0);
}

void destroy_shader_program(D3D11Device *device, Resource shader_program)
{
	switch (resource_type(shader_program)) {
		case RESOURCE_VERTEX_SHADER:
		{
			VertexShader *vs = vertex_shader(device, shader_program);
			ID3D11VertexShader_Release(vs->shader);
			release_vertex_shader_handle(device, shader_program);
		}
		break;
		case RESOURCE_PIXEL_SHADER:
		{
			PixelShader *ps = pixel_shader(device, shader_program);
			ID3D11PixelShader_Release(ps->shader);
			release_pixel_shader_handle(device, shader_program);
		}
		break;
		default:
			assert(0);
			break;
	}
}

InputLayout *input_layout(D3D11Device *device, Resource vs_res, Resource vd_res)
{
	UINT64 *hashes = device->resources.input_layout_hashes;
	const unsigned n_layouts = sb_count(hashes);
	UINT64 hash = vs_res.handle;
	hash = hash << 32;
	hash |= vd_res.handle;
	for (unsigned i = 0; i < n_layouts; ++i) {
		if (hashes[i] != hash)
			continue;

		return &device->resources.input_layouts[i];
	}

	sb_push(hashes, hash);
	InputLayout in_layout = { .input_layout = NULL };

	VertexShader *vs = vertex_shader(device, vs_res);
	VertexDeclaration *vd = vertex_declaration(device, vd_res);

	ID3D11Device_CreateInputLayout(device->device, vd->elements, sb_count(vd->elements), ID3D10Blob_GetBufferPointer(vs->bytecode), ID3D10Blob_GetBufferSize(vs->bytecode), &in_layout.input_layout);

	sb_push(device->resources.input_layouts, in_layout);

	return &sb_last(device->resources.input_layouts);
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
	static unsigned color_index = 0;
	color_index = color_index++ % 3;
	struct Color {
		float rgba[4];
	};
	struct Color colors[] = {
		{ 1.0, 0.0, 0.0, 1.0 },
		{ 0.0, 1.0, 0.0, 1.0 },
		{ 0.0, 0.0, 1.0, 1.0 },
		{ 0.0, 0.0, 0.0, 1.0 }
	};
	ID3D11DeviceContext_ClearRenderTargetView(device->immediate_context, device->resources.swap_chain_rtv, colors[3].rgba);

	Resource vs_res, vd_res;
	VertexBuffer *vb = NULL;
	VertexDeclaration *vd = NULL;
	VertexShader *vs = NULL;
	PixelShader *ps = NULL;
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
			vd_res = resource;
			break;
		case RESOURCE_VERTEX_SHADER:
			vs = vertex_shader(device, resource);
			vs_res = resource;
			break;
		case RESOURCE_PIXEL_SHADER:
			ps = pixel_shader(device, resource);
			break;
		}
	}

	InputLayout *in_layout = input_layout(device, vs_res, vd_res);

	ID3D11DeviceContext_IASetInputLayout(device->immediate_context, in_layout->input_layout);
	ID3D11DeviceContext_OMSetRenderTargets(device->immediate_context, 1, &device->resources.swap_chain_rtv, NULL);
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

	UINT stride = 16;
	UINT offset = 0;
	ID3D11DeviceContext_IASetVertexBuffers(device->immediate_context, 0, 1, &vb->buffer, &stride, &offset);

	ID3D11DeviceContext_OMSetDepthStencilState(device->immediate_context, device->resources.depth_stencil_state, 0);
	ID3D11DeviceContext_OMSetBlendState(device->immediate_context, device->resources.blend_state, 0, 0xFFFFFFFFU);
	ID3D11DeviceContext_RSSetState(device->immediate_context, device->resources.rasterizer_state);

	ID3D11DeviceContext_Draw(device->immediate_context, 3, 0);
}