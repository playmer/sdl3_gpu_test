#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdbool.h>

// Utilities
void report_error(const char* format, ...)
{
	puts("Dev Error: ");
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\nSDL Error: %s", SDL_GetError());
}

typedef struct render_context {
	SDL_GpuDevice* device;
}render_context;

bool init_render_context(render_context* context, SDL_Window* window)
{
	context->device = SDL_GpuCreateDevice(SDL_GPU_BACKEND_ALL, 1, 0);

	if (NULL == context->device)
	{
		report_error("Failed create device, exiting...");
		return false;
	}

	if (!SDL_GpuClaimWindow(
		context->device,
		window,
		SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
		SDL_GPU_PRESENTMODE_VSYNC))
	{
		report_error("Failed claim window, exiting...");
		return false;
	}

	return true;
}

typedef struct VertexPosition2D
{
	float x, y; /* 3D data. Vertex range -0.5..0.5 in all axes. Z -0.5 is near, 0.5 is far. */
} VertexPosition2D;

typedef struct VertexPosition3D
{
	float x, y, z; /* 3D data. Vertex range -0.5..0.5 in all axes. Z -0.5 is near, 0.5 is far. */
} VertexPosition3D;

typedef struct VertexColor
{
	float r, g, b;  /* red, green, blue; intensity 0 to 1 (alpha is always 1). */
} VertexColor;

// Example 1: Triangle
typedef struct example_1 {

	SDL_GpuGraphicsPipeline* pipeline;
	SDL_GpuBuffer* buffer_position;
	SDL_GpuBuffer* buffer_color;
}example_1;


static const VertexPosition2D example_1_position_data[] = {
	{ 0.f, 1.f},
	{ 1.f,-1.f},
	{-1.f,-1.f},
};

static const VertexColor example_1_color_data[] = {
	{1.f, 0.f, 0.f},
	{0.f, 1.f, 0.f},
	{0.f, 0.f, 1.f},
};

bool create_buffer_and_upload(
	render_context* context,
	SDL_GpuCommandBuffer* command_buffer,
	SDL_GpuCopyPass* copy_pass,
	SDL_GpuBuffer** buffer_destination,
	SDL_GpuTransferBuffer** buffer_transfer,
	const char* name,
	void const* data,
	size_t size)
{
	*buffer_destination = SDL_GpuCreateBuffer(
		context->device,
		SDL_GPU_BUFFERUSAGE_VERTEX_BIT,
		size);
	if (NULL == *buffer_destination)
	{
		report_error("Failed to create a buffer, exiting...");
		return false;
	}

	SDL_GpuSetBufferName(
		context->device,
		*buffer_destination,
		"example_1_position_data");

	*buffer_transfer = SDL_GpuCreateTransferBuffer(
		context->device,
		SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		size);

	if (NULL == *buffer_transfer)
	{
		report_error("Failed to create a transfer buffer, exiting...");
		return false;
	}

	SDL_GpuTransferBufferRegion region;
	region.transferBuffer = *buffer_transfer;
	region.offset = 0;
	region.size = size;
	SDL_GpuSetTransferData(context->device, data, &region, SDL_FALSE);

	SDL_GpuTransferBufferLocation buffer_location;
	buffer_location.transferBuffer = *buffer_transfer;
	buffer_location.offset = 0;
	SDL_GpuBufferRegion dst_region;
	dst_region.buffer = *buffer_destination;
	dst_region.offset = 0;
	dst_region.size = size;
	SDL_GpuUploadToBuffer(copy_pass, &buffer_location, &dst_region, SDL_FALSE);

	return true;
}

typedef enum ShaderType {
	ShaderType_Vertex,
	ShaderType_Fragment,
	ShaderType_Compute
}ShaderType;

SDL_GpuShader* load_shader(render_context* context, const char* name, ShaderType type)
{
	SDL_GpuShaderCreateInfo createinfo;
	createinfo.samplerCount = 0;
	createinfo.storageBufferCount = 0;
	createinfo.storageTextureCount = 0;
	createinfo.uniformBufferCount = 0;

	const char* shader_type_string = NULL;
	switch (type)
	{
		case ShaderType_Vertex: 
			shader_type_string = ".vert";
			createinfo.entryPointName = "vtx_main";
			break;
		case ShaderType_Fragment:
			shader_type_string = ".frag";
			createinfo.entryPointName = "frag_main";
			break;
		case ShaderType_Compute:
			shader_type_string = ".comp";
			createinfo.entryPointName = "comp_main";
			break;
		default:
			puts("Passed an invalid `ShaderType` to `load_shader`, exiting...");
			return NULL;
	}

	SDL_GpuBackend backend = SDL_GpuGetBackend(context->device);
	const char* shader_backend_string = NULL;
	
	switch (backend)
	{
		case SDL_GPU_BACKEND_D3D11:
		//case SDL_GPU_BACKEND_D3D12:
			shader_type_string = ".hlsl";
			createinfo.format = SDL_GPU_SHADERFORMAT_HLSL;
			break;
		case SDL_GPU_BACKEND_VULKAN:
			shader_type_string = ".spirv";
			createinfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
			break;
		case SDL_GPU_BACKEND_METAL:
			shader_type_string = ".metallib";
			createinfo.format = SDL_GPU_SHADERFORMAT_METALLIB;
			break;
		default:
			puts("Running on a currently unsupported SDL_gpu backend, exiting...");
			return NULL;
	}

	char filename_buffer[256];
	SDL_snprintf(filename_buffer, sizeof(filename_buffer), "%s.%s.%s", name, shader_type_string, shader_backend_string);

	return NULL;
}

bool init_example_1(render_context* context, SDL_Window* window, example_1* example)
{
	SDL_GpuCommandBuffer* command_buffer = SDL_GpuAcquireCommandBuffer(context->device);
	if (NULL == command_buffer)
	{
		report_error("Failed to create a command buffer, exiting...");
		return false;
	}

	SDL_GpuCopyPass* copy_pass = SDL_GpuBeginCopyPass(command_buffer);
	if (NULL == copy_pass)
	{
		report_error("Failed to create a command buffer, exiting...");
		return false;
	}

	SDL_GpuTransferBuffer* buffer_position_transfer;
	if (!create_buffer_and_upload(
		context,
		command_buffer,
		copy_pass,
		&example->buffer_position,
		&buffer_position_transfer,
		"example_1_position_data",
		example_1_position_data,
		sizeof(example_1_position_data)))
	{
		// Already printed an error, just fail out.
		return false;
	}

	SDL_GpuTransferBuffer* buffer_color_transfer;
	if (!create_buffer_and_upload(
		context,
		command_buffer,
		copy_pass,
		&example->buffer_color,
		&buffer_color_transfer,
		"example_1_color_data",
		example_1_color_data,
		sizeof(example_1_color_data)))
	{
		// Already printed an error, just fail out.
		return false;
	}

	SDL_GpuEndCopyPass(copy_pass);
	SDL_GpuSubmit(command_buffer);

	SDL_GpuReleaseTransferBuffer(context->device, buffer_position_transfer);
	SDL_GpuReleaseTransferBuffer(context->device, buffer_color_transfer);


	SDL_GpuShader* vertex_shader = load_shader(context, "example_1", ShaderType_Vertex);
	SDL_GpuShader* fragment_shader = load_shader(context, "example_1", ShaderType_Fragment);







	/* Set up the graphics pipeline */
	SDL_GpuColorAttachmentDescription color_attachment_desc;
	SDL_zero(color_attachment_desc);
	color_attachment_desc.format = SDL_GpuGetSwapchainTextureFormat(context->device, window);

	color_attachment_desc.blendState.blendEnable = 0;
	color_attachment_desc.blendState.alphaBlendOp = SDL_GPU_BLENDOP_ADD;
	color_attachment_desc.blendState.colorBlendOp = SDL_GPU_BLENDOP_ADD;
	color_attachment_desc.blendState.colorWriteMask = 0xF;
	color_attachment_desc.blendState.srcAlphaBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
	color_attachment_desc.blendState.dstAlphaBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;
	color_attachment_desc.blendState.srcColorBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
	color_attachment_desc.blendState.dstColorBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;

	SDL_GpuGraphicsPipelineCreateInfo pipelinedesc;
	SDL_zero(pipelinedesc);

	pipelinedesc.attachmentInfo.colorAttachmentCount = 1;
	pipelinedesc.attachmentInfo.colorAttachmentDescriptions = &color_attachment_desc;
	pipelinedesc.attachmentInfo.depthStencilFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
	pipelinedesc.attachmentInfo.hasDepthStencilAttachment = SDL_TRUE;

	pipelinedesc.depthStencilState.depthTestEnable = 1;
	pipelinedesc.depthStencilState.depthWriteEnable = 1;
	pipelinedesc.depthStencilState.compareOp = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

	pipelinedesc.multisampleState.multisampleCount = SDL_GPU_SAMPLECOUNT_1;
	pipelinedesc.multisampleState.sampleMask = 0xF;

	pipelinedesc.primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

	pipelinedesc.vertexShader = vertex_shader;
	pipelinedesc.fragmentShader = fragment_shader;

	SDL_GpuVertexBinding vertex_binding[2];
	vertex_binding[0].binding = 0;
	vertex_binding[0].inputRate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	vertex_binding[0].stepRate = 0;
	vertex_binding[0].stride = sizeof(VertexPosition2D);
	vertex_binding[1].binding = 1;
	vertex_binding[1].inputRate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	vertex_binding[1].stepRate = 0;
	vertex_binding[1].stride = sizeof(VertexColor);

	SDL_GpuVertexAttribute vertex_attributes[2];

	// Position
	vertex_attributes[0].binding = 0;
	vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_VECTOR2;
	vertex_attributes[0].location = 0;
	vertex_attributes[0].offset = 0;

	// Color
	vertex_attributes[1].binding = 1;
	vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_VECTOR3;
	vertex_attributes[1].location = 0;
	vertex_attributes[1].offset = 0;

	pipelinedesc.vertexInputState.vertexBindingCount = 2;
	pipelinedesc.vertexInputState.vertexBindings = &vertex_binding;
	pipelinedesc.vertexInputState.vertexAttributeCount = 2;
	pipelinedesc.vertexInputState.vertexAttributes = vertex_attributes;

	example->pipeline = SDL_GpuCreateGraphicsPipeline(context->device, &pipelinedesc);
	if (NULL == example->pipeline)
	{
		report_error("Failed to create a pipeline, exiting...");
		return false;
	}

	/* These are reference-counted; once the pipeline is created, you don't need to keep these. */
	SDL_GpuReleaseShader(context->device, vertex_shader);
	SDL_GpuReleaseShader(context->device, fragment_shader);





















	return true;
}

void render_example_1(render_context* context, SDL_Window* window, example_1* example)
{
	SDL_GpuTexture* swapchain;



	SDL_GpuCommandBuffer* command_buffer = SDL_GpuAcquireCommandBuffer(context->device);
	if (NULL == command_buffer)
	{
		report_error("Failed to create a command buffer, exiting...");
		return false;
	}

	Uint32 drawablew, drawableh;
	swapchain = SDL_GpuAcquireSwapchainTexture(command_buffer, window, &drawablew, &drawableh);

	if (!swapchain) {
		/* No swapchain was acquired, probably too many frames in flight */
		SDL_GpuSubmit(command_buffer);
		return;
	}

	SDL_GpuColorAttachmentInfo color_attachment;
	SDL_zero(color_attachment);
	color_attachment.clearColor.a = 1.0f;
	color_attachment.loadOp = SDL_GPU_LOADOP_CLEAR;
	color_attachment.storeOp = SDL_GPU_STOREOP_STORE;
	color_attachment.textureSlice.texture = swapchain;

	SDL_GpuRenderPass* render_pass = SDL_GpuBeginRenderPass(command_buffer, &color_attachment, 1, NULL);

	SDL_GpuBindGraphicsPipeline(render_pass, example->pipeline);

	SDL_GpuBufferBinding position_vertex_binding;
	position_vertex_binding.buffer = example->buffer_position;
	position_vertex_binding.offset = 0;
	SDL_GpuBufferBinding color_vertex_binding;
	position_vertex_binding.buffer = example->buffer_color;
	position_vertex_binding.offset = 0;
	SDL_GpuBindVertexBuffers(render_pass, 0, &position_vertex_binding, 1);
	SDL_GpuBindVertexBuffers(render_pass, 0, &color_vertex_binding, 1);

	SDL_GpuDrawPrimitives(render_pass, 0, 3);

	SDL_GpuEndRenderPass(render_pass);
	SDL_GpuSubmit(command_buffer);
}

int main(int argc, char** argv)
{
	if (0 != SDL_Init(SDL_INIT_VIDEO))
	{
		report_error("Failed to initialize SDL, exiting...");
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow("LearnGPU", 800, 600, 0);

	if (NULL == window)
	{
		report_error("Failed to initialize SDL, exiting...");
		return 1;
	}

	render_context context;
	if (!init_render_context(&context, window))
	{
		report_error("Failed to initialize the render context, exiting...");
		return 1;
	}

	example_1 example_1;
	if (!init_example_1(&context, window, &example_1))
	{
		report_error("Failed to initialize example 1, exiting...");
		return 1;
	}

	bool running = true;
	while (running)
	{
		SDL_Event event;

		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_EVENT_QUIT:
					running = false;
					break;
			}
		}

		render_example_1(&context, window, &example_1);
	}

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}