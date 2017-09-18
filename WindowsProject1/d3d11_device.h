#pragma once
#define CINTERFACE
#define COBJMACROS

#include <windows.h>

typedef struct D3D11Device D3D11Device;
typedef struct Allocator Allocator;

int initialize_d3d11_device(Allocator *allocator, HWND window, D3D11Device **d3d11_device);

void shutdown_d3d11_device(Allocator *allocator, D3D11Device *d3d11_device);

int d3d11_device_update(D3D11Device *device);

typedef struct Resource
{
	UINT32 handle;
} Resource;

enum ResourceTypes { RESOURCE_NOT_INITIALIZED = 0, RESOURCE_VERTEX_BUFFER, RESOURCE_VERTEX_DECLARATION };

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

Resource create_vertex_buffer(struct D3D11Device *device, void *buffer, unsigned vertices, unsigned stride);
void destroy_vertex_buffer(struct D3D11Device *device, Resource resource);

typedef struct VertexElement
{
	unsigned semantic;
	unsigned type;
} VertexElement_t;

enum VertexSemantic { VS_POSITION, VS_COLOR };
enum VertexType { VT_FLOAT3, VT_FLOAT4 };
Resource create_vertex_declaration(struct D3D11Device *device, VertexElement_t *vertex_elements, unsigned n_vertex_elements);
void destroy_vertex_declaration(struct D3D11Device *device, Resource resource);