#pragma once
#define CINTERFACE
#define COBJMACROS

#include <windows.h>

typedef struct D3D11Device D3D11Device;
typedef struct Allocator Allocator;
typedef struct RenderPackage RenderPackage;

int initialize_d3d11_device(Allocator *allocator, HWND window, D3D11Device **d3d11_device);

void shutdown_d3d11_device(Allocator *allocator, D3D11Device *d3d11_device);

void d3d11_device_render(D3D11Device *device, RenderPackage *render_package);

int d3d11_device_present(D3D11Device *device);

typedef struct Resource
{
	UINT32 handle;
} Resource;

enum ResourceTypes { RESOURCE_NOT_INITIALIZED = 0, RESOURCE_VERTEX_BUFFER, RESOURCE_VERTEX_DECLARATION, RESOURCE_VERTEX_SHADER, RESOURCE_PIXEL_SHADER };

inline unsigned resource_type(Resource resource)
{
	return (resource.handle & 0xFF000000U) >> 24U;
}

inline unsigned resource_handle(Resource resource)
{
	return resource.handle & 0x00FFFFFFU;
}

inline Resource resource_encode_handle_type(unsigned handle, unsigned type)
{
	struct Resource resource = { .handle = (handle & 0x00FFFFFFU) | ((type & 0xFFU) << 24U) };
	return resource;
}

inline BOOL resource_is_valid(Resource resource)
{
	return resource_handle(resource) != RESOURCE_NOT_INITIALIZED;
}

Resource create_vertex_buffer(D3D11Device *device, void *buffer, unsigned vertices, unsigned stride);
void destroy_vertex_buffer(D3D11Device *device, Resource resource);

typedef struct VertexElement
{
	unsigned semantic;
	unsigned type;
} VertexElement_t;

enum VertexSemantic { VS_POSITION, VS_COLOR };
enum VertexType { VT_FLOAT3, VT_FLOAT4 };
Resource create_vertex_declaration(D3D11Device *device, VertexElement_t *vertex_elements, unsigned n_vertex_elements);
void destroy_vertex_declaration(D3D11Device *device, Resource resource);

enum ShaderProgramType { SPT_VERTEX = 0, SPT_PIXEL };
Resource create_shader_program(D3D11Device *device, unsigned shader_program_type, const char *program, unsigned program_length);
void destroy_shader_program(D3D11Device *device, Resource shader_program);

RenderPackage *create_render_package(Allocator *allocator, const Resource *resources, unsigned n_resources);
void destroy_render_package(RenderPackage *render_package);