#include "render_resources.h"

#define CINTERFACE
#define COBJMACROS

#include <d3d11.h>

#include <dxgi.h>
#include <assert.h>
#include <d3dcompiler.h>
#include "stretchy_buffer.h"

struct RenderResources
{
	Allocator *allocator;
	ID3D11Device *d3d_device;

	Buffer *vertex_buffers;
	unsigned *free_vertex_buffers;

	Buffer *index_buffers;
	unsigned *free_index_buffers;

	RawBuffer *raw_buffers;
	unsigned *free_raw_buffers;

	VertexDeclaration *vertex_declarations;
	unsigned *free_vertex_declarations;

	VertexShader *vertex_shaders;
	unsigned *free_vertex_shaders;

	PixelShader *pixel_shaders;
	unsigned *free_pixel_shaders;

	InputLayout *input_layouts;
	UINT64 *input_layout_hashes;
};

Resource render_resources_allocate_vertex_buffer_handle(RenderResources *resources)
{
	if (sb_count(resources->free_vertex_buffers)) {
		unsigned h = sb_last(resources->free_vertex_buffers);
		sb_pop(resources->free_vertex_buffers);
		return resource_encode_handle_type(h, RESOURCE_VERTEX_BUFFER);
	}

	unsigned h = sb_count(resources->vertex_buffers);
	Buffer buffer = { .buffer = NULL };
	sb_push(resources->vertex_buffers, buffer);

	return resource_encode_handle_type(h, RESOURCE_VERTEX_BUFFER);
}

void render_resources_release_vertex_buffer_handle(RenderResources *resources, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_BUFFER);
	unsigned h = resource_handle(resource);
	sb_push(resources->free_vertex_buffers, h);
}

Resource render_resources_allocate_index_buffer_handle(RenderResources *resources)
{
	if (sb_count(resources->free_index_buffers)) {
		unsigned h = sb_last(resources->free_index_buffers);
		sb_pop(resources->free_index_buffers);
		return resource_encode_handle_type(h, RESOURCE_INDEX_BUFFER);
	}

	unsigned h = sb_count(resources->index_buffers);
	Buffer buffer = { .buffer = NULL };
	sb_push(resources->index_buffers, buffer);

	return resource_encode_handle_type(h, RESOURCE_INDEX_BUFFER);
}

void render_resources_release_index_buffer_handle(RenderResources *resources, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_INDEX_BUFFER);
	unsigned h = resource_handle(resource);
	sb_push(resources->free_index_buffers, h);
}

Resource render_resources_allocate_raw_buffer_handle(RenderResources *resources)
{
	if (sb_count(resources->free_raw_buffers)) {
		unsigned h = sb_last(resources->free_raw_buffers);
		sb_pop(resources->free_raw_buffers);
		return resource_encode_handle_type(h, RESOURCE_RAW_BUFFER);
	}

	unsigned h = sb_count(resources->raw_buffers);
	RawBuffer buffer = { .buffer = NULL, .srv = NULL };
	sb_push(resources->raw_buffers, buffer);

	return resource_encode_handle_type(h, RESOURCE_RAW_BUFFER);
}

void render_resources_release_raw_buffer_handle(RenderResources *resources, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_RAW_BUFFER);
	unsigned h = resource_handle(resource);
	sb_push(resources->free_raw_buffers, h);
}

Resource render_resources_allocate_vertex_declaration_handle(RenderResources *resources)
{
	if (sb_count(resources->free_vertex_declarations)) {
		unsigned h = sb_last(resources->free_vertex_declarations);
		sb_pop(resources->free_vertex_declarations);
		return resource_encode_handle_type(h, RESOURCE_VERTEX_DECLARATION);
	}

	unsigned h = sb_count(resources->vertex_declarations);
	VertexDeclaration vertex_declaration = { .elements = NULL };
	sb_push(resources->vertex_declarations, vertex_declaration);

	return resource_encode_handle_type(h, RESOURCE_VERTEX_DECLARATION);
}

void render_resources_release_vertex_declaration_handle(RenderResources *resources, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_DECLARATION);
	unsigned h = resource_handle(resource);
	sb_push(resources->free_vertex_declarations, h);
}

Resource render_resources_allocate_vertex_shader_handle(RenderResources *resources)
{
	if (sb_count(resources->free_vertex_shaders)) {
		unsigned h = sb_last(resources->free_vertex_shaders);
		sb_pop(resources->free_vertex_shaders);
		return resource_encode_handle_type(h, RESOURCE_VERTEX_SHADER);
	}

	unsigned h = sb_count(resources->vertex_shaders);
	VertexShader vertex_shader = { .shader = NULL };
	sb_push(resources->vertex_shaders, vertex_shader);

	return resource_encode_handle_type(h, RESOURCE_VERTEX_SHADER);
}

void render_resources_release_vertex_shader_handle(RenderResources *resources, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_SHADER);
	unsigned h = resource_handle(resource);
	sb_push(resources->free_vertex_shaders, h);
}

Resource render_resources_allocate_pixel_shader_handle(RenderResources *resources)
{
	if (sb_count(resources->free_pixel_shaders)) {
		unsigned h = sb_last(resources->free_pixel_shaders);
		sb_pop(resources->free_pixel_shaders);
		return resource_encode_handle_type(h, RESOURCE_PIXEL_SHADER);
	}

	unsigned h = sb_count(resources->pixel_shaders);
	PixelShader pixel_shader = { .shader = NULL };
	sb_push(resources->pixel_shaders, pixel_shader);

	return resource_encode_handle_type(h, RESOURCE_PIXEL_SHADER);
}

void render_resources_release_pixel_shader_handle(RenderResources *resources, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_PIXEL_SHADER);
	unsigned h = resource_handle(resource);
	sb_push(resources->free_pixel_shaders, h);
}

void render_resources_create(Allocator *allocator, ID3D11Device *d3d_device, RenderResources **out_resources)
{
	RenderResources *resources = *out_resources = allocator_realloc(allocator, NULL, sizeof(RenderResources), 16);
	resources->allocator = allocator;
	resources->d3d_device = d3d_device;

	{
		sb_create(allocator, resources->vertex_buffers, 10);
		sb_create(allocator, resources->free_vertex_buffers, 10);
		Resource first_vb = render_resources_allocate_vertex_buffer_handle(resources);
		assert(resource_handle(first_vb) == 0);
		(void)first_vb;
	}

	{
		sb_create(allocator, resources->index_buffers, 10);
		sb_create(allocator, resources->free_index_buffers, 10);
		Resource first_ib = render_resources_allocate_index_buffer_handle(resources);
		assert(resource_handle(first_ib) == 0);
		(void)first_ib;
	}

	{
		sb_create(allocator, resources->raw_buffers, 10);
		sb_create(allocator, resources->free_raw_buffers, 10);
		Resource first_rb = render_resources_allocate_raw_buffer_handle(resources);
		assert(resource_handle(first_rb) == 0);
		(void)first_rb;
	}

	{
		sb_create(allocator, resources->vertex_declarations, 10);
		sb_create(allocator, resources->free_vertex_declarations, 10);
		Resource first_vd = render_resources_allocate_vertex_declaration_handle(resources);
		assert(resource_handle(first_vd) == 0);
		(void)first_vd;
	}

	{
		sb_create(allocator, resources->vertex_shaders, 10);
		sb_create(allocator, resources->free_vertex_shaders, 10);
		Resource first_vs = render_resources_allocate_vertex_shader_handle(resources);
		assert(resource_handle(first_vs) == 0);
		(void)first_vs;
	}

	{
		sb_create(allocator, resources->pixel_shaders, 10);
		sb_create(allocator, resources->free_pixel_shaders, 10);
		Resource first_ps = render_resources_allocate_pixel_shader_handle(resources);
		assert(resource_handle(first_ps) == 0);
		(void)first_ps;
	}
	{
		sb_create(allocator, resources->input_layouts, 10);
		sb_create(allocator, resources->input_layout_hashes, 10);
	}
}

void render_resources_destroy(Allocator *allocator, RenderResources *resources)
{
	render_resources_release_vertex_buffer_handle(resources, resource_encode_handle_type(0, RESOURCE_VERTEX_BUFFER));
	render_resources_release_vertex_declaration_handle(resources, resource_encode_handle_type(0, RESOURCE_VERTEX_DECLARATION));
	render_resources_release_vertex_shader_handle(resources, resource_encode_handle_type(0, RESOURCE_VERTEX_SHADER));
	render_resources_release_pixel_shader_handle(resources, resource_encode_handle_type(0, RESOURCE_PIXEL_SHADER));

	const unsigned n_input_layouts = sb_count(resources->input_layouts);
	for (unsigned i = 0; i < n_input_layouts; ++i) {
		ID3D11InputLayout_Release(resources->input_layouts[i].input_layout);
	}

	sb_free(resources->vertex_buffers);
	sb_free(resources->free_vertex_buffers);
	sb_free(resources->index_buffers);
	sb_free(resources->free_index_buffers);
	sb_free(resources->raw_buffers);
	sb_free(resources->free_raw_buffers);
	sb_free(resources->vertex_declarations);
	sb_free(resources->free_vertex_declarations);
	sb_free(resources->vertex_shaders);
	sb_free(resources->free_vertex_shaders);
	sb_free(resources->pixel_shaders);
	sb_free(resources->free_pixel_shaders);
	sb_free(resources->input_layouts);
	sb_free(resources->input_layout_hashes);

	allocator_realloc(allocator, resources, 0, 0);
}

Buffer *render_resources_vertex_buffer(RenderResources *resources, Resource resource)
{
	return &resources->vertex_buffers[resource_handle(resource)];
}

Resource render_resources_create_vertex_buffer(RenderResources *resources, void *buffer, unsigned vertices, unsigned stride)
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

	Resource vb_res = render_resources_allocate_vertex_buffer_handle(resources);

	Buffer *vb = render_resources_vertex_buffer(resources, vb_res);
	vb->stride = stride;

	HRESULT hr = ID3D11Device_CreateBuffer(resources->d3d_device, &desc, buffer ? &sub_desc : 0, &vb->buffer);
	assert(SUCCEEDED(hr));

	ID3D11Buffer_QueryInterface(vb->buffer, &IID_ID3D11Resource, &vb->resource);

	return vb_res;
}

void render_resources_destroy_vertex_buffer(RenderResources *resources, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_BUFFER);

	Buffer *vb = render_resources_vertex_buffer(resources, resource);
	ID3D11Buffer_Release(vb->buffer);
	ID3D11Resource_Release(vb->resource);

	render_resources_release_vertex_buffer_handle(resources, resource);
}

void render_resource_vertex_buffer_update(RenderResources *resources, Resource resource, void *buffer, unsigned size)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_BUFFER);
	Buffer *vb = render_resources_vertex_buffer(resources, resource);

	ID3D11DeviceContext *immediate_context = NULL;
	ID3D11Device_GetImmediateContext(resources->d3d_device, &immediate_context);
	D3D11_BOX dest_box;
	dest_box.left = 0;
	dest_box.right = size;
	dest_box.top = 0;
	dest_box.bottom = 1;
	dest_box.front = 0;
	dest_box.back = 1;
	ID3D11DeviceContext_UpdateSubresource(immediate_context, vb->resource, 0, &dest_box, buffer, size, 0);
	ID3D11DeviceContext_Release(immediate_context);
}

Buffer *render_resources_index_buffer(RenderResources *resources, Resource resource)
{
	return &resources->index_buffers[resource_handle(resource)];
}

Resource render_resources_create_index_buffer(RenderResources *resources, void *buffer, unsigned indices, unsigned stride)
{
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = indices * stride;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA sub_desc;
	sub_desc.SysMemPitch = 0;
	sub_desc.SysMemSlicePitch = 0;
	sub_desc.pSysMem = buffer;

	Resource ib_res = render_resources_allocate_index_buffer_handle(resources);

	Buffer *vb = render_resources_index_buffer(resources, ib_res);
	vb->stride = stride;

	HRESULT hr = ID3D11Device_CreateBuffer(resources->d3d_device, &desc, buffer ? &sub_desc : 0, &vb->buffer);
	assert(SUCCEEDED(hr));

	return ib_res;
}

void render_resources_destroy_index_buffer(RenderResources *resources, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_INDEX_BUFFER);

	Buffer *ib = render_resources_index_buffer(resources, resource);
	ID3D11Buffer_Release(ib->buffer);

	render_resources_release_index_buffer_handle(resources, resource);
}

RawBuffer *render_resources_raw_buffer(RenderResources *resources, Resource resource)
{
	return &resources->raw_buffers[resource_handle(resource)];
}

Resource render_resources_create_raw_buffer(RenderResources *resources, void *buffer, unsigned size)
{
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = size;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA sub_desc;
	sub_desc.SysMemPitch = 0;
	sub_desc.SysMemSlicePitch = 0;
	sub_desc.pSysMem = buffer;

	Resource rb_res = render_resources_allocate_raw_buffer_handle(resources);

	RawBuffer *rb = render_resources_raw_buffer(resources, rb_res);
	HRESULT hr = ID3D11Device_CreateBuffer(resources->d3d_device, &desc, buffer ? &sub_desc : 0, &rb->buffer);
	assert(SUCCEEDED(hr));

	ID3D11Buffer_QueryInterface(rb->buffer, &IID_ID3D11Resource, &rb->resource);

	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
	srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srv_desc.BufferEx.FirstElement = 0;
	srv_desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
	srv_desc.BufferEx.NumElements = size / sizeof(unsigned);
	hr = ID3D11Device_CreateShaderResourceView(resources->d3d_device, rb->resource, &srv_desc, &rb->srv);

	return rb_res;
}

void render_resources_destroy_raw_buffer(RenderResources *resources, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_RAW_BUFFER);

	RawBuffer *rb = render_resources_raw_buffer(resources, resource);
	ID3D11Resource_Release(rb->resource);
	ID3D11Buffer_Release(rb->buffer);
	ID3D11ShaderResourceView_Release(rb->srv);

	render_resources_release_raw_buffer_handle(resources, resource);
}

void render_resource_raw_buffer_update(RenderResources *resources, Resource resource, void *buffer, unsigned size)
{
	assert(resource_type(resource) == RESOURCE_RAW_BUFFER);
	RawBuffer *rb = render_resources_raw_buffer(resources, resource);

	ID3D11DeviceContext *immediate_context;
	ID3D11Device_GetImmediateContext(resources->d3d_device, &immediate_context);
	D3D11_BOX dest_box;
	dest_box.left = 0;
	dest_box.right = size;
	dest_box.top = 0;
	dest_box.bottom = 1;
	dest_box.front = 0;
	dest_box.back = 1;
	ID3D11DeviceContext_UpdateSubresource(immediate_context, rb->resource, 0, &dest_box, buffer, size, 0);
	ID3D11DeviceContext_Release(immediate_context);
}

VertexDeclaration *render_resources_vertex_declaration(RenderResources *resources, Resource resource)
{
	return &resources->vertex_declarations[resource_handle(resource)];
}

Resource render_resources_create_vertex_declaration(RenderResources *resources, VertexElement *vertex_elements, unsigned n_vertex_elements)
{
	Resource vd_res = render_resources_allocate_vertex_declaration_handle(resources);

	VertexDeclaration *vd = render_resources_vertex_declaration(resources, vd_res);

	sb_create(resources->allocator, vd->elements, n_vertex_elements);

	static const char* semantics[] = { "POSITION", "COLOR", "TEXCOORD" };
	static const DXGI_FORMAT formats[] = { DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R8G8B8A8_UNORM };
	static unsigned type_size[] = {
		3 * sizeof(float),
		4 * sizeof(float),
		4 * sizeof(char),
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

void render_resources_destroy_vertex_declaration(RenderResources *resources, Resource resource)
{
	assert(resource_type(resource) == RESOURCE_VERTEX_DECLARATION);

	VertexDeclaration *vd = render_resources_vertex_declaration(resources, resource);
	sb_free(vd->elements);

	render_resources_release_vertex_declaration_handle(resources, resource);
}

VertexShader *render_resources_vertex_shader(RenderResources *resources, Resource resource)
{
	return &resources->vertex_shaders[resource_handle(resource)];
}

PixelShader *render_resources_pixel_shader(RenderResources *resources, Resource resource)
{
	return &resources->pixel_shaders[resource_handle(resource)];
}

Resource render_resources_create_shader_program(RenderResources *resources, unsigned shader_program_type, const char *program, unsigned program_length)
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
		Resource shader = render_resources_allocate_vertex_shader_handle(resources);
		VertexShader *vs = render_resources_vertex_shader(resources, shader);
		vs->bytecode = shader_program;
		hr = ID3D11Device_CreateVertexShader(resources->d3d_device, ID3D10Blob_GetBufferPointer(shader_program), ID3D10Blob_GetBufferSize(shader_program), NULL, &vs->shader);
		assert(SUCCEEDED(hr));
		return shader;
	}
	break;
	case SPT_PIXEL:
	{
		Resource shader = render_resources_allocate_pixel_shader_handle(resources);
		PixelShader *ps = render_resources_pixel_shader(resources, shader);
		hr = ID3D11Device_CreatePixelShader(resources->d3d_device, ID3D10Blob_GetBufferPointer(shader_program), ID3D10Blob_GetBufferSize(shader_program), NULL, &ps->shader);
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

void render_resources_destroy_shader_program(RenderResources *resources, Resource shader_program)
{
	switch (resource_type(shader_program)) {
	case RESOURCE_VERTEX_SHADER:
	{
		VertexShader *vs = render_resources_vertex_shader(resources, shader_program);
		ID3D11VertexShader_Release(vs->shader);
		render_resources_release_vertex_shader_handle(resources, shader_program);
	}
	break;
	case RESOURCE_PIXEL_SHADER:
	{
		PixelShader *ps = render_resources_pixel_shader(resources, shader_program);
		ID3D11PixelShader_Release(ps->shader);
		render_resources_release_pixel_shader_handle(resources, shader_program);
	}
	break;
	default:
		assert(0);
		break;
	}
}

InputLayout *render_resources_input_layout(RenderResources *resources, Resource vs_res, Resource vd_res)
{
	UINT64 *hashes = resources->input_layout_hashes;
	const unsigned n_layouts = sb_count(hashes);
	UINT64 hash = vs_res.handle;
	hash = hash << 32;
	hash |= vd_res.handle;
	for (unsigned i = 0; i < n_layouts; ++i) {
		if (hashes[i] != hash)
			continue;

		return &resources->input_layouts[i];
	}

	sb_push(hashes, hash);
	InputLayout in_layout = { .input_layout = NULL };

	VertexShader *vs = render_resources_vertex_shader(resources, vs_res);
	VertexDeclaration *vd = render_resources_vertex_declaration(resources, vd_res);
	const unsigned n_elements = sb_count(vd->elements);
	ID3D11Device_CreateInputLayout(resources->d3d_device, vd->elements, n_elements, ID3D10Blob_GetBufferPointer(vs->bytecode), ID3D10Blob_GetBufferSize(vs->bytecode), &in_layout.input_layout);

	sb_push(resources->input_layouts, in_layout);

	return &sb_last(resources->input_layouts);
}

RenderPackage *create_render_package(Allocator *allocator, const Resource *resources, unsigned n_resources, unsigned n_vertices, unsigned n_indices)
{
	RenderPackage *package = (RenderPackage*)allocator_realloc(allocator, NULL, sizeof(RenderPackage) + sizeof(Resource) * n_resources, 16);
	package->allocator = allocator;
	package->n_resources = n_resources;
	package->resources = (Resource*)((uintptr_t)package + sizeof(RenderPackage));
	memcpy(package->resources, resources, sizeof(Resource) * n_resources);
	package->n_vertices = n_vertices;
	package->n_indices = n_indices;
	package->n_instances = 1;

	return package;
}

void destroy_render_package(RenderPackage *render_package)
{
	allocator_realloc(render_package->allocator, render_package, 0, 0);
}