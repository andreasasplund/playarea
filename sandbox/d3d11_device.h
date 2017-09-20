#pragma once

typedef struct D3D11Device D3D11Device;
typedef struct Allocator Allocator;
typedef struct RenderPackage RenderPackage;
typedef struct RenderResources RenderResources;

void d3d11_device_create(Allocator *allocator, void *window, D3D11Device **d3d11_device);
void d3d11_device_destroy(Allocator *allocator, D3D11Device *d3d11_device);

RenderResources *d3d11_device_render_resources(D3D11Device *device);

void d3d11_device_clear(D3D11Device *device);
void d3d11_device_render(D3D11Device *device, RenderPackage *render_package);
void d3d11_device_present(D3D11Device *device);