#pragma once
#define CINTERFACE
#define COBJMACROS

#include <windows.h>

int initialize_d3d11_device(struct Allocator *allocator, HWND window, struct D3D11Device **d3d11_device);

void shutdown_d3d11_device(struct Allocator *allocator, struct D3D11Device *d3d11_device);

int d3d11_device_update(struct D3D11Device *device);

typedef struct Resource
{
	UINT32 handle;
} Resource_t;

enum ResourceTypes { RESOURCE_NOT_INITIALIZED = 0, RESOURCE_VERTEX_BUFFER = 1, RESOURCE_INDEX_BUFFER = 2 };

inline unsigned resource_type(Resource_t resource)
{
	return (resource.handle & 0xFF000000U) >> 24U;
}

inline unsigned resource_handle(Resource_t resource)
{
	return resource.handle & 0x00FFFFFFU;
}

inline Resource_t resource_encode_handle_type(unsigned handle, unsigned type)
{
	struct Resource resource = { .handle = (handle & 0x00FFFFFFU) | ((type & 0xFFU) << 24U) };
	return resource;
}

inline BOOL resource_is_valid(Resource_t resource)
{
	return resource_handle(resource) != RESOURCE_NOT_INITIALIZED;
}

Resource_t create_vertex_buffer(struct D3D11Device *device, void *buffer, unsigned vertices, unsigned stride);
void destroy_vertex_buffer(struct D3D11Device *device, Resource_t resource);