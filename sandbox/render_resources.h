#pragma once

typedef struct ID3D11Device ID3D11Device;
typedef struct ID3D11Buffer ID3D11Buffer;
typedef struct D3D11_INPUT_ELEMENT_DESC D3D11_INPUT_ELEMENT_DESC;
typedef struct ID3D11ShaderResourceView ID3D11ShaderResourceView;
typedef struct ID3D11VertexShader ID3D11VertexShader;
typedef struct ID3D11PixelShader ID3D11PixelShader;
typedef struct ID3D10Blob ID3D10Blob;
typedef ID3D10Blob ID3DBlob;
typedef struct ID3D11InputLayout ID3D11InputLayout;

typedef struct RenderResources RenderResources;
typedef struct Allocator Allocator;

void render_resources_create(Allocator *allocator, ID3D11Device *d3d_device, RenderResources **resources);
void render_resources_destroy(Allocator *allocator, RenderResources *resources);

typedef struct Resource
{
	unsigned handle;
} Resource;

enum ResourceTypes { RESOURCE_NOT_INITIALIZED = 0, RESOURCE_VERTEX_BUFFER, RESOURCE_INDEX_BUFFER, RESOURCE_VERTEX_DECLARATION, RESOURCE_VERTEX_SHADER, RESOURCE_PIXEL_SHADER, RESOURCE_RAW_BUFFER };

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

typedef struct RawBuffer
{
	ID3D11Buffer *buffer;
	ID3D11ShaderResourceView *srv;
} RawBuffer;

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

Buffer *render_resources_vertex_buffer(RenderResources *resources, Resource resource);
Resource render_resources_create_vertex_buffer(RenderResources *resources, void *buffer, unsigned vertices, unsigned stride);
void render_resources_destroy_vertex_buffer(RenderResources *resources, Resource resource);

Buffer *render_resources_index_buffer(RenderResources *resources, Resource resource);
Resource render_resources_create_index_buffer(RenderResources *resources, void *buffer, unsigned indices, unsigned stride);
void render_resources_destroy_index_buffer(RenderResources *resources, Resource resource);

RawBuffer *render_resources_raw_buffer(RenderResources *resources, Resource resource);
Resource render_resources_create_raw_buffer(RenderResources *resources, void *buffer, unsigned size);
void render_resources_destroy_raw_buffer(RenderResources *resources, Resource resource);

typedef struct VertexElement
{
	unsigned semantic;
	unsigned type;
} VertexElement;
enum VertexSemantic { VS_POSITION, VS_COLOR, VS_TEXCOORD };
enum VertexType { VT_FLOAT3, VT_FLOAT4 };
VertexDeclaration *render_resources_vertex_declaration(RenderResources *resources, Resource resource);
Resource render_resources_create_vertex_declaration(RenderResources *resources, VertexElement *vertex_elements, unsigned n_vertex_elements);
void render_resources_destroy_vertex_declaration(RenderResources *resources, Resource resource);

enum ShaderProgramType { SPT_VERTEX = 0, SPT_PIXEL };
VertexShader *render_resources_vertex_shader(RenderResources *resources, Resource resource);
PixelShader *render_resources_pixel_shader(RenderResources *resources, Resource resource);
Resource render_resources_create_shader_program(RenderResources *resources, unsigned shader_program_type, const char *program, unsigned program_length);
void render_resources_destroy_shader_program(RenderResources *resources, Resource shader_program);

InputLayout *render_resources_input_layout(RenderResources *resources, Resource vertex_stream_resource, Resource vertex_declaration_resource);
typedef struct RenderPackage
{
	Allocator *allocator;
	Resource *resources;
	unsigned n_resources;
	unsigned n_vertices;
	unsigned n_indices;
	unsigned n_instances;
} RenderPackage;

RenderPackage *create_render_package(Allocator *allocator, const Resource *resources, unsigned n_resources, unsigned n_vertices, unsigned n_indices);
void destroy_render_package(RenderPackage *render_package);