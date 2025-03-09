#include "pch.h"
#include "vk_renderer.h"

#include <SDL2/SDL_vulkan.h>


void Renderer::init()
{
	init_window();

	init_vulkan();

	init_scene();
}

void Renderer::run()
{
	while (m_running)
	{
		delta_time();

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT) {
				m_running = false;
			}
			if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
				SDL_SetRelativeMouseMode(SDL_TRUE);
				m_main_camera.allowMovement = true;
				m_main_camera.firstMouse = true;
			}
			if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
				SDL_SetRelativeMouseMode(SDL_FALSE);
				m_main_camera.allowMovement = false;
			}
			if (event.type == SDL_MOUSEMOTION && m_main_camera.allowMovement) {
				// handle_mouse_motion(event.motion.xrel, event.motion.yrel);
				if (m_main_camera.firstMouse)
				{
					m_main_camera.lastX = event.motion.xrel;
					m_main_camera.lastY = event.motion.yrel;
					m_main_camera.firstMouse = false;
				}

				float xoffset = event.motion.xrel;
				float yoffset = -event.motion.yrel;

				float sensitivity = 0.1f;
				xoffset *= sensitivity;
				yoffset *= sensitivity;

				m_main_camera.yaw += xoffset;
				m_main_camera.pitch += yoffset;

				if (m_main_camera.pitch > 89.0f)
					m_main_camera.pitch = 89.0f;
				if (m_main_camera.pitch < -89.0f)
					m_main_camera.pitch = -89.0f;

				glm::vec3 front;
				front.x = cos(glm::radians(m_main_camera.yaw)) * cos(glm::radians(m_main_camera.pitch));
				front.y = sin(glm::radians(m_main_camera.pitch));
				front.z = sin(glm::radians(m_main_camera.yaw)) * cos(glm::radians(m_main_camera.pitch));
				m_main_camera.front = glm::normalize(front);
			}
		}

		draw();
	}

	shutdown();
}

void Renderer::shutdown()
{
	vkDeviceWaitIdle(m_device);

	m_main_deletor.flush();

	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	vmaDestroyAllocator(m_allocator);
	vkDestroyDevice(m_device, nullptr);

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);

	SDL_DestroyWindow(m_window);
	SDL_Quit();
}

void Renderer::init_window()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
	}

	m_window = SDL_CreateWindow("jvsc_vulkan engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_window_width, m_window_height, SDL_WINDOW_VULKAN);

	if (!m_window)
	{
		SDL_Quit();
		throw std::runtime_error("Failed to create window");
	}

	m_window_extent = { (uint32_t)m_window_width, (uint32_t)m_window_height };
}

void Renderer::init_vulkan()
{
	std::vector<const char*> required_instance_extensions = {

	};

	std::vector<const char*> required_layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	init_instance(required_instance_extensions, required_layers);

	create_surface();

	std::vector<const char*> required_device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
	};

	VkPhysicalDeviceFeatures required_features = {
	
	};

	init_device(required_device_extensions, required_features);

	init_allocator();

	create_swapchain();

	create_image_views();

	create_default_renderpass();

	create_framebuffers();

	init_commands();

	init_sync_objects();

	init_descriptors();

	init_pipeline();
}

void Renderer::init_instance(std::vector<const char*>& extensions, std::vector<const char*>& layers)
{
	auto SDL_required = get_required_vulkan_extensions();
	extensions.insert(extensions.end(), SDL_required.begin(), SDL_required.end());

	if (!check_instance_extensions_support(extensions))
	{
		throw std::runtime_error("Required extensions are not supported!");
	}

	if (!check_layers_support(layers))
	{
		throw std::runtime_error("Required layers are not supported!");
	}

	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Vulkan Engine",
		.pEngineName = "No Engine",
		.apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0)
	};

	VkInstanceCreateInfo instance_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledLayerCount = static_cast<uint32_t>(layers.size()),
		.ppEnabledLayerNames = layers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
	};

	VK_CHECK(vkCreateInstance(&instance_info, nullptr, &m_instance));
}

bool Renderer::check_instance_extensions_support(std::vector<const char*>& required_extensions)
{
	uint32_t available_extension_count;
	vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr);
	
	std::vector<VkExtensionProperties> available_extensions(available_extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, available_extensions.data());

	std::set<std::string> available_extensions_set;
	for (const auto& extension : available_extensions)
	{
		available_extensions_set.insert(extension.extensionName);
	}

	for (const auto& required_extension : required_extensions)
	{
		if (available_extensions_set.find(required_extension) == available_extensions_set.end())
		{
			return false;
		}
	}
	return true;

}

bool Renderer::check_layers_support(std::vector<const char*>& required_layers)
{
	uint32_t available_layers_count;
	vkEnumerateInstanceLayerProperties(&available_layers_count, nullptr);

	std::vector<VkLayerProperties> available_layers(available_layers_count);
	vkEnumerateInstanceLayerProperties(&available_layers_count, available_layers.data());

	std::set<std::string> available_layers_set;
	for (const auto& layer : available_layers)
	{
		available_layers_set.insert(layer.layerName);
	}

	for (const auto& required_layer : required_layers)
	{
		if (available_layers_set.find(required_layer) == available_layers_set.end())
		{
			return false;
		}
	}
	return true;
}

std::vector<const char*> Renderer::get_required_vulkan_extensions()
{
	uint32_t count;
	SDL_Vulkan_GetInstanceExtensions(m_window, &count, nullptr);

	std::vector<const char*> extensions(count);
	SDL_Vulkan_GetInstanceExtensions(m_window, &count, extensions.data());

	return extensions;
}

void Renderer::create_surface()
{
	SDL_Vulkan_CreateSurface(m_window, m_instance, &m_surface);
}

void Renderer::init_device(std::vector<const char*>& device_extensions, VkPhysicalDeviceFeatures enabled_features)
{
	select_physical_device();

	if (!check_device_extensions_support(device_extensions))
	{
		throw std::runtime_error("Required device extensions are not supported!");
	}

	VkDeviceQueueCreateInfo queue_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = m_graphics_family_index,
		.queueCount = 1,
		.pQueuePriorities = new float(1.0f),
	};

	VkDeviceCreateInfo device_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queue_info,
		.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
		.ppEnabledExtensionNames = device_extensions.data(),
		.pEnabledFeatures = &enabled_features
	};

	VK_CHECK(vkCreateDevice(m_gpu, &device_info, nullptr, &m_device));

	vkGetDeviceQueue(m_device, m_graphics_family_index, 0, &m_graphics_queue);

}

void Renderer::select_physical_device()
{
	uint32_t gpu_count = 0;
	vkEnumeratePhysicalDevices(m_instance, &gpu_count, nullptr);

	if (gpu_count == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> gpus(gpu_count);
	vkEnumeratePhysicalDevices(m_instance, &gpu_count, gpus.data());
	
	m_gpu = gpus[0];

	vkGetPhysicalDeviceProperties(m_gpu, &m_gpu_properties);
	std::cout << "The GPU has a minimum buffer alignment of " << m_gpu_properties.limits.minUniformBufferOffsetAlignment << '\n';

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &queue_family_count, queue_families.data());

	std::optional<uint32_t> graphicsQueueFamily;
	for (uint32_t i = 0; i < queue_family_count; ++i) 
	{
		VkBool32 supports_present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu, i, m_surface, &supports_present);

		if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supports_present) {
			graphicsQueueFamily = i;
			break;
		}
	}

	if (!graphicsQueueFamily) 
	{
		throw std::runtime_error("Failed to find a queue family with graphics support!");
	}

	m_graphics_family_index = graphicsQueueFamily.value();
}

bool Renderer::check_device_extensions_support(std::vector<const char*>& required_device_extensions)
{
	uint32_t available_extensions_count;
	vkEnumerateDeviceExtensionProperties(m_gpu, nullptr, &available_extensions_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(available_extensions_count);
	vkEnumerateDeviceExtensionProperties(m_gpu, nullptr, &available_extensions_count, available_extensions.data());

	std::set<std::string> available_extensions_set;
	for (const auto& extension : available_extensions)
	{
		available_extensions_set.insert(extension.extensionName);
	}

	for (const auto& required_extension : required_device_extensions)
	{
		if (available_extensions_set.find(required_extension) == available_extensions_set.end())
		{
			return false;
		}
	}
	return true;
}

void Renderer::init_allocator()
{
	VmaAllocatorCreateInfo allocator_info = {
		.physicalDevice = m_gpu,
		.device = m_device,
		.instance = m_instance
	};

	VK_CHECK(vmaCreateAllocator(&allocator_info, &m_allocator));
}

void Renderer::create_swapchain()
{
	VkSurfaceCapabilitiesKHR capabilities = get_surface_capabilities();
	std::vector<VkSurfaceFormatKHR> available_surface_formats = get_surface_formats();
	std::vector<VkPresentModeKHR> available_present_modes = get_present_modes();

	VkSurfaceFormatKHR surface_format = choose_surface_format(available_surface_formats);
	VkPresentModeKHR present_mode = choose_present_mode(available_present_modes);
	VkExtent2D extent = choose_swap_extent(capabilities, static_cast<uint32_t>(m_window_width), static_cast<uint32_t>(m_window_height));

	uint32_t image_count = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
	{
		image_count = capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchain_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = m_surface,
		.minImageCount = image_count,
		.imageFormat = surface_format.format,
		.imageColorSpace = surface_format.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = nullptr
	};

	VK_CHECK(vkCreateSwapchainKHR(m_device, &swapchain_info, nullptr, &m_swapchain));

	m_swapchain_format = surface_format.format;
}

VkSurfaceCapabilitiesKHR Renderer::get_surface_capabilities()
{
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_gpu, m_surface, &capabilities);

	return capabilities;
}

std::vector<VkSurfaceFormatKHR> Renderer::get_surface_formats()
{
	uint32_t count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu, m_surface, &count, nullptr);

	std::vector<VkSurfaceFormatKHR> formats(count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu, m_surface, &count, formats.data());

	return formats;
}

std::vector<VkPresentModeKHR> Renderer::get_present_modes()
{
	uint32_t count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_gpu, m_surface, &count, nullptr);

	std::vector<VkPresentModeKHR> present_modes(count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_gpu, m_surface, &count, present_modes.data());

	return present_modes;
}

VkPresentModeKHR Renderer::choose_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
	for (const auto& availablePresentMode : available_present_modes) 
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
		{
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR Renderer::choose_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
	for (const auto& availableFormat : available_formats) 
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
		{
			return availableFormat;
		}
	}
	return available_formats[0];
}

VkExtent2D Renderer::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
{
	if (capabilities.currentExtent.width != UINT32_MAX) 
	{
		return capabilities.currentExtent;
	}
	else 
	{
		VkExtent2D actualExtent = { width, height };

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

void Renderer::create_image_views()
{
	uint32_t image_count;
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, nullptr);

	std::vector<VkImage> swapchain_images(image_count);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, swapchain_images.data());

	m_swapchain_images = swapchain_images;

	for (size_t i = 0; i < image_count; i++)
	{
		VkImageViewCreateInfo view_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = swapchain_images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = m_swapchain_format,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		VkImageView image_view;
		VK_CHECK(vkCreateImageView(m_device, &view_info, nullptr, &image_view));
		m_swapchain_image_views.push_back(image_view);
	}

}

void Renderer::create_default_renderpass()
{
	VkAttachmentDescription color_attachment_description = {
		.format = m_swapchain_format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference color_attachment_ref = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref,
	};

	std::array<VkAttachmentDescription, 1> attachments = { color_attachment_description };

	VkSubpassDependency color_dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	};

	std::array<VkSubpassDependency, 1> dependencies = { color_dependency };

	VkRenderPassCreateInfo renderpass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = static_cast<uint32_t>(dependencies.size()),
		.pDependencies = dependencies.data()
	};

	VK_CHECK(vkCreateRenderPass(m_device, &renderpass_info, nullptr, &m_renderpass));

	m_main_deletor.add([=]()
	{
			vkDestroyRenderPass(m_device, m_renderpass, nullptr);
	});
}

void Renderer::init_pipeline()
{
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = { {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = load_shader_module("assets/shaders/spirv/default_vert.spv"),
			.pName = "main"
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = load_shader_module("assets/shaders/spirv/default_frag.spv"),
			.pName = "main"
		}
	} };

	VertexInputDescription vertex_description = Vertex::get_vertex_description();

	VkPipelineVertexInputStateCreateInfo vertex_input = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_description.bindings.size()),
		.pVertexBindingDescriptions = vertex_description.bindings.data(),
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_description.attributes.size()),
		.pVertexAttributeDescriptions = vertex_description.attributes.data()
	};

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	VkPipelineViewportStateCreateInfo viewport = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1
	};

	VkPipelineRasterizationStateCreateInfo raster = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f
	};

	VkPipelineMultisampleStateCreateInfo multisample = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
	};

	VkPipelineColorBlendAttachmentState blend_attachment = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo blend = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &blend_attachment
	};

	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
		.pDynamicStates = dynamic_states.data()
	};

	VkPipelineLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &m_global_set_layout
	};

	VK_CHECK(vkCreatePipelineLayout(m_device, &layout_info, nullptr, &m_pipeline_layout));

	VkGraphicsPipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>(shader_stages.size()),
		.pStages = shader_stages.data(),
		.pVertexInputState = &vertex_input,
		.pInputAssemblyState = &input_assembly,
		.pViewportState = &viewport,
		.pRasterizationState = &raster,
		.pMultisampleState = &multisample,
		.pColorBlendState = &blend,
		.pDynamicState = &dynamic_state_info,
		.layout = m_pipeline_layout,
		.renderPass = m_renderpass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE
	};

	VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_pipeline));

	m_main_deletor.add([=]()
	{
		vkDestroyPipeline(m_device, m_pipeline, nullptr);
		vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
	});

	vkDestroyShaderModule(m_device, shader_stages[0].module, nullptr);
	vkDestroyShaderModule(m_device, shader_stages[1].module, nullptr);
}

VkShaderModule Renderer::load_shader_module(const char* file_path)
{
	std::ifstream file(file_path, std::ios::ate | std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("Failed to open file");

	size_t file_size = static_cast<size_t>(file.tellg());

	std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));
	file.seekg(0);

	file.read((char*)buffer.data(), file_size);
	file.close();

	VkShaderModuleCreateInfo module_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = file_size,
		.pCode = buffer.data()
	};

	VkShaderModule shader_module;
	vkCreateShaderModule(m_device, &module_info, nullptr, &shader_module);

	return shader_module;
}

void Renderer::init_scene()
{
	m_main_camera.init(m_window_extent.width, m_window_extent.height, { 0.0f, 0.0f, 10.0f });

	std::vector<Vertex> vertices = {
		{{ 0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }},
		{{ 0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
		{{-0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
	};

	VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

	m_triangle_buffer = create_buffer(buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data;
	vmaMapMemory(m_allocator, m_triangle_buffer.allocation, &data);
		memcpy(data, vertices.data(), (size_t)buffer_size);
	vmaUnmapMemory(m_allocator, m_triangle_buffer.allocation);

	m_main_deletor.add([=]()
	{
		vmaDestroyBuffer(m_allocator, m_triangle_buffer.buffer, m_triangle_buffer.allocation);
	});
}

void Renderer::draw()
{
	VK_CHECK(vkWaitForFences(m_device, 1, &get_current_frame().render_fence, VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(m_device, 1, &get_current_frame().render_fence));

	uint32_t swapchain_image_index;
	VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, get_current_frame().present_semaphore, nullptr, &swapchain_image_index));

	VK_CHECK(vkResetCommandBuffer(get_current_frame().main_command_buffer, 0));
	VkCommandBuffer cmd = get_current_frame().main_command_buffer;

	VkCommandBufferBeginInfo cmd_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

	VkClearValue clear;
	clear.color = { {0.02f, 0.02f, 0.02f, 1.0f} };

	std::array<VkClearValue, 1> clear_values = { clear };

	VkRenderPassBeginInfo renderpass_begin_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = m_renderpass,
		.framebuffer = m_framebuffers[swapchain_image_index],
		.renderArea = {
			.offset = { 0, 0 },
			.extent = m_window_extent,
		},
		.clearValueCount = static_cast<uint32_t>(clear_values.size()),
		.pClearValues = clear_values.data()
	};

	vkCmdBeginRenderPass(cmd, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	m_main_camera.update(m_window, m_delta_time);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
	
	VkViewport viewport = {
		.width = static_cast<float>(m_window_extent.width),
		.height = static_cast<float>(m_window_extent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor = {
		.extent = {
			.width = m_window_extent.width,
			.height = m_window_extent.height
		}
	};

	vkCmdSetScissor(cmd, 0, 1, &scissor);

	GPUCameraData cam_data = {
		.view = m_main_camera.view,
		.proj = m_main_camera.projection,
		.viewproj = m_main_camera.projection * m_main_camera.get_view_matrix(),
	};

	void* data;
	vmaMapMemory(m_allocator, get_current_frame().camera_buffer.allocation, &data);
	memcpy(data, &cam_data, sizeof(GPUCameraData));
	vmaUnmapMemory(m_allocator, get_current_frame().camera_buffer.allocation);

	uint32_t uniform_offset = { 0 };
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &get_current_frame().global_descriptor, 0, nullptr);

	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, &m_triangle_buffer.buffer, offsets);

	vkCmdDraw(cmd, 3, 1, 0, 0);

	vkCmdEndRenderPass(cmd);
	VK_CHECK(vkEndCommandBuffer(cmd));

	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &get_current_frame().present_semaphore,
		.pWaitDstStageMask = &wait_stage,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &get_current_frame().render_semaphore,
	};

	VK_CHECK(vkQueueSubmit(m_graphics_queue, 1, &submit_info, get_current_frame().render_fence));

	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &get_current_frame().render_semaphore,
		.swapchainCount = 1,
		.pSwapchains = &m_swapchain,
		.pImageIndices = &swapchain_image_index
	};

	VK_CHECK(vkQueuePresentKHR(m_graphics_queue, &present_info));

	m_frame_number++;
}

AllocatedBuffer Renderer::create_buffer(size_t buffer_size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags memory_flags)
{
	AllocatedBuffer new_buffer;

	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = buffer_size,
		.usage = buffer_usage
	};

	VmaAllocationCreateInfo alloc_info = {
		.flags = memory_flags,
		.usage = memory_usage
	};

	VK_CHECK(vmaCreateBuffer(m_allocator, &buffer_info, &alloc_info, &new_buffer.buffer, &new_buffer.allocation, nullptr));

	return new_buffer;
}

void Renderer::delta_time()
{
	Uint32 current_frame = SDL_GetTicks();
	m_delta_time = (current_frame - m_last_frame) / 10000.0f;
	m_last_frame = current_frame;
}

void Renderer::create_framebuffers()
{
	VkFramebufferCreateInfo framebuffer_info = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = m_renderpass,
		.attachmentCount = 1,
		.width = static_cast<uint32_t>(m_window_width),
		.height = static_cast<uint32_t>(m_window_height),
		.layers = 1
	};

	m_framebuffers.resize(m_swapchain_images.size());
	for (int i = 0; i < m_swapchain_images.size(); i++)
	{
		std::array<VkImageView, 1> attachments;
		attachments[0] = m_swapchain_image_views[i];

		framebuffer_info.pAttachments = attachments.data();
		VK_CHECK(vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &m_framebuffers[i]));

		m_main_deletor.add([=]()
		{
			vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);
			vkDestroyImageView(m_device, m_swapchain_image_views[i], nullptr);
		});
	}
}

void Renderer::init_commands()
{
	
	VkCommandPoolCreateInfo cmd_poll_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = m_graphics_family_index,
	};
		
	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateCommandPool(m_device, &cmd_poll_info, nullptr, &m_frames[i].command_pool));

		VkCommandBufferAllocateInfo cmd_buffer_alloc_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = m_frames[i].command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
			
		VK_CHECK(vkAllocateCommandBuffers(m_device, &cmd_buffer_alloc_info, &m_frames[i].main_command_buffer));
		m_main_deletor.add([=, this]()
		{
			vkDestroyCommandPool(m_device, m_frames[i].command_pool, nullptr);
		});
	}
}

void Renderer::init_sync_objects()
{
	VkFenceCreateInfo fenceInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	VkSemaphoreCreateInfo semaphoreInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &m_frames[i].render_fence));
		VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_frames[i].render_semaphore));
		VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_frames[i].present_semaphore));
		m_main_deletor.add([=]()
			{
				vkDestroySemaphore(m_device, m_frames[i].present_semaphore, nullptr);
				vkDestroySemaphore(m_device, m_frames[i].render_semaphore, nullptr);
				vkDestroyFence(m_device, m_frames[i].render_fence, nullptr);
			});
	}
}

void Renderer::init_descriptors()
{
	create_descriptor_set_layout();

	create_descriptor_pool();

	alloc_descriptor_sets();
}

void Renderer::create_descriptor_set_layout()
{
	VkDescriptorSetLayoutBinding camera_buffer_binding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
	};

	VkDescriptorSetLayoutCreateInfo layout_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &camera_buffer_binding,
	};

	VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layout_info, nullptr, &m_global_set_layout));

	m_main_deletor.add([=]()
	{
		vkDestroyDescriptorSetLayout(m_device, m_global_set_layout, nullptr);
	});
}

void Renderer::create_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> sizes = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
	};

	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 10,
		.poolSizeCount = static_cast<uint32_t>(sizes.size()),
		.pPoolSizes = sizes.data(),
	};

	VK_CHECK(vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_descriptor_pool));

	m_main_deletor.add([=]()
	{
		vkDestroyDescriptorPool(m_device, m_descriptor_pool, nullptr);
	});
}

void Renderer::alloc_descriptor_sets()
{
	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		m_frames[i].camera_buffer = create_buffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		VkDescriptorSetAllocateInfo alloc_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = m_descriptor_pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &m_global_set_layout
		};

		VK_CHECK(vkAllocateDescriptorSets(m_device, &alloc_info, &m_frames[i].global_descriptor));

		VkDescriptorBufferInfo buffer_info = {
			.buffer = m_frames[i].camera_buffer.buffer,
			.offset = 0,
			.range = sizeof(GPUCameraData)
		};

		VkWriteDescriptorSet camera_write = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = m_frames[i].global_descriptor,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &buffer_info,
		};


		vkUpdateDescriptorSets(m_device, 1, &camera_write, 0, nullptr);

		m_main_deletor.add([=]()
		{
			vmaDestroyBuffer(m_allocator, m_frames[i].camera_buffer.buffer, m_frames[i].camera_buffer.allocation);
		});
	}
}
