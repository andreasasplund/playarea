#pragma once
#define CINTERFACE
#include <windows.h>

int initialize_d3d11_device(struct Allocator *allocator, HWND window, struct D3D11Device **d3d11_device);

void shutdown_d3d11_device(struct Allocator *allocator, struct D3D11Device *d3d11_device);

int d3d11_device_update(struct D3D11Device *device);