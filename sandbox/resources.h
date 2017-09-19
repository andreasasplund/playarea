#pragma once

#define CINTERFACE
#define COBJMACROS

#include <d3d11.h>

typedef struct Resources Resources;
typedef struct Allocator Allocator;

void create_resources(Allocator *allocator, ID3D11Device *d3d_device, Resources **resources);
void destroy_resources(Allocator *allocator, Resources *resources);

typedef struct Resource
{
	unsigned handle;
} Resource;

enum ResourceTypes { RESOURCE_NOT_INITIALIZED = 0, RESOURCE_VERTEX_BUFFER, RESOURCE_INDEX_BUFFER, RESOURCE_VERTEX_DECLARATION, RESOURCE_VERTEX_SHADER, RESOURCE_PIXEL_SHADER };

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
	Resource resource = { .handle = (handle & 0x00FFFFFFU) | ((type & 0xFFU) << 24U) };
	return resource;
}

inline int resource_is_valid(Resource resource)
{
	return resource_handle(resource) != RESOURCE_NOT_INITIALIZED;
}

typedef struct Buffer
{
	ID3D11Buffer *buffer;
	unsigned stride;
} Buffer;

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

Buffer *vertex_buffer(Resources *resources, Resource resource);
Resource create_vertex_buffer(Resources *resources, void *buffer, unsigned vertices, unsigned stride);
void destroy_vertex_buffer(Resources *resources, Resource resource);

Buffer *index_buffer(Resources *resources, Resource resource);
Resource create_index_buffer(Resources *resources, void *buffer, unsigned indices, unsigned stride);
void destroy_index_buffer(Resources *resources, Resource resource);

typedef struct VertexElement
{
	unsigned semantic;
	unsigned type;
} VertexElement;
enum VertexSemantic { VS_POSITION, VS_COLOR, VS_TEXCOORD };
enum VertexType { VT_FLOAT3, VT_FLOAT4 };
VertexDeclaration *vertex_declaration(Resources *resources, Resource resource);
Resource create_vertex_declaration(Resources *resources, VertexElement *vertex_elements, unsigned n_vertex_elements);
void destroy_vertex_declaration(Resources *resources, Resource resource);

enum ShaderProgramType { SPT_VERTEX = 0, SPT_PIXEL };
VertexShader *vertex_shader(Resources *resources, Resource resource);
PixelShader *pixel_shader(Resources *resources, Resource resource);
Resource create_shader_program(Resources *resources, unsigned shader_program_type, const char *program, unsigned program_length);
void destroy_shader_program(Resources *resources, Resource shader_program);

InputLayout *input_layout(Resources *resources, Resource vertex_stream_resource, Resource vertex_declaration_resource);
typedef struct RenderPackage
{
	Allocator *allocator;
	Resource *resources;
	unsigned n_resources;
	unsigned n_vertices;
	unsigned n_indices;
} RenderPackage;

RenderPackage *create_render_package(Allocator *allocator, const Resource *resources, unsigned n_resources, unsigned n_vertices, unsigned n_indices);
void destroy_render_package(RenderPackage *render_package);