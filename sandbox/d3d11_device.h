#pragma once
#define CINTERFACE
#define COBJMACROS

#include <windows.h>

typedef struct D3D11Device D3D11Device;
typedef struct Allocator Allocator;
typedef struct RenderPackage RenderPackage;
typedef struct Resources Resources;

int initialize_d3d11_device(Allocator *allocator, HWND window, D3D11Device **d3d11_device);
void shutdown_d3d11_device(Allocator *allocator, D3D11Device *d3d11_device);

Resources *d3d11_resources(D3D11Device *device);

void d3d11_device_clear(D3D11Device *device);
void d3d11_device_render(D3D11Device *device, RenderPackage *render_package);
int d3d11_device_present(D3D11Device *device);