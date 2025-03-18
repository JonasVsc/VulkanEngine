#include "jvsc_renderer.hpp"

// lib
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>


// std
#include <unordered_set>
#include <set>
#include <stdexcept>
#include <iostream>

namespace jvsc {

	// local callback functions
	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}

	void destroy_debug_utils_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,const VkAllocationCallbacks* pAllocator) 
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			instance,
			"vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}

	JvscRenderer::JvscRenderer(JvscWindow& window)
		: m_window{window}
	{
		std::cout << "calling renderer constructor" << '\n';

		create_instance();
		create_surface();
		select_physical_device();
		create_device();
		create_allocator();
		create_swapchain();
		create_swapchain_image_views();
		create_depth_resources();
		create_render_pass();
		create_framebuffers();
		create_sync_objects();
		create_command_pool();
		create_command_buffers();
	}

	void JvscRenderer::terminate()
	{
		std::cout << "calling renderer destructor" << '\n';

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_device, m_render_finished_semaphores[i], nullptr);
			vkDestroySemaphore(m_device, m_image_available_semaphores[i], nullptr);
			vkDestroyFence(m_device, m_in_flight_fences[i], nullptr);
		}

		for (size_t i = 0; i < m_swapchain_images.size(); i++)
		{
			vkDestroyFramebuffer(m_device, m_swapchain_framebuffers[i], nullptr);
			vkDestroyImageView(m_device, m_depth_image_views[i], nullptr);
			vkDestroyImageView(m_device, m_swapchain_image_views[i], nullptr);
			vmaDestroyImage(m_allocator, m_depth_images[i], m_depth_image_allocations[i]);
		}
		
		m_depth_image_allocations.clear();
		m_swapchain_framebuffers.clear();
		m_depth_image_views.clear();
		m_swapchain_image_views.clear();
		m_swapchain_images.clear();
		m_depth_images.clear();

		vkDestroyRenderPass(m_device, m_render_pass, nullptr);
		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

		vkFreeCommandBuffers(m_device, m_command_pool, m_command_buffers.size(), m_command_buffers.data());
		m_command_buffers.clear();

		vkDestroyCommandPool(m_device, m_command_pool, nullptr);
		vmaDestroyAllocator(m_allocator);
		vkDestroyDevice(m_device, nullptr);

		// if (enable_validation_layers) { destroy_debug_utils_messenger(m_instance, m_debug_messenger, nullptr); }

		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyInstance(m_instance, nullptr);
	}

	VkResult JvscRenderer::submit_command_buffers(uint32_t* image_index)
	{
		if (m_images_in_flight[*image_index] != VK_NULL_HANDLE)
			vkWaitForFences(m_device, 1, &m_images_in_flight[*image_index], VK_TRUE, UINT64_MAX);
		
		m_images_in_flight[*image_index] = m_in_flight_fences[m_current_frame];

		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore wait_semaphores[] = { m_image_available_semaphores[m_current_frame] };
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_semaphores;
		submit_info.pWaitDstStageMask = wait_stages;

		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &m_command_buffers[*image_index];

		VkSemaphore signal_semaphores[] = { m_render_finished_semaphores[m_current_frame] };
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_semaphores;

		vkResetFences(m_device, 1, &m_in_flight_fences[m_current_frame]);
		if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, m_in_flight_fences[m_current_frame]) != VK_SUCCESS)
			throw std::runtime_error("failed to submit draw command buffer!");

		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_semaphores;

		VkSwapchainKHR swapchains[] = { m_swapchain };
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swapchains;

		present_info.pImageIndices = image_index;

		auto result = vkQueuePresentKHR(m_present_queue, &present_info);

		m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

		return result;
	}

	VkCommandBuffer JvscRenderer::begin_frame()
	{
		vkWaitForFences(m_device, 1, &m_in_flight_fences[m_current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());
		VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, std::numeric_limits<uint64_t>::max(), m_image_available_semaphores[m_current_frame], VK_NULL_HANDLE, &m_image_index);
	
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			while (m_swapchain_extent.width == 0 || m_swapchain_extent.height == 0)
			{
				handle_minimize();
			}
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			throw std::runtime_error("failed to acquire swapchain image");

		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(m_command_buffers[m_image_index], &begin_info) != VK_SUCCESS)
			throw std::runtime_error("failed to begin recording command buffer");

		return m_command_buffers[m_image_index];
	}

	void JvscRenderer::begin_swapchain_render_pass(VkCommandBuffer cmd)
	{
		VkRenderPassBeginInfo render_info{};
		render_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_info.renderPass = m_render_pass;
		render_info.framebuffer = m_swapchain_framebuffers[m_image_index];

		render_info.renderArea.offset = { 0, 0 };
		render_info.renderArea.extent = m_swapchain_extent;

		std::vector<VkClearValue> clear_values(2, {});
		clear_values[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
		clear_values[1].depthStencil = { 1.0f, 0 };

		render_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
		render_info.pClearValues = clear_values.data();
		vkCmdBeginRenderPass(cmd, &render_info, VK_SUBPASS_CONTENTS_INLINE);
	}

	void JvscRenderer::end_frame(VkCommandBuffer cmd)
	{
		if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
			throw std::runtime_error("failed to record command buffer");

		VkResult result = submit_command_buffers(&m_image_index);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			handle_minimize();
			return;
		}
		if (result != VK_SUCCESS)
			throw std::runtime_error("failed to present swapchain image");
	}

	void JvscRenderer::handle_minimize()
	{
		auto extent = m_window.get_extent();
		while (extent.width == 0 || extent.height == 0)
		{
			extent = m_window.get_extent();
			glfwWaitEvents();
		}
	}


	void JvscRenderer::create_instance()
	{
		VkApplicationInfo app_info = {};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "JvscVulkanEngine App";
		app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
		app_info.pEngineName = "JvscVulkanEngine";
		app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
		app_info.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo instance_info = {};
		instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_info.pApplicationInfo = &app_info;

		auto extensions = get_required_extensions();
		instance_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		instance_info.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
		if (enable_validation_layers) 
		{
			instance_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
			instance_info.ppEnabledLayerNames = validation_layers.data();

			debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debug_info.pfnUserCallback = debug_callback;
			debug_info.pUserData = nullptr;  // Optional

			instance_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_info;
		}
		else 
		{
			instance_info.enabledLayerCount = 0;
			instance_info.pNext = nullptr;
		}

		if (vkCreateInstance(&instance_info, nullptr, &m_instance) != VK_SUCCESS)
			throw std::runtime_error("failed to create instance!");

		glfw_required_extensions();
	}

	void JvscRenderer::create_surface()
	{
		m_window.create_surface(m_instance, &m_surface);
	}

	void JvscRenderer::select_physical_device()
	{
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);
		if (device_count == 0)
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		
		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

		for (const auto& device : devices) 
		{
			if (is_device_suitable(device)) 
			{
				m_physical_device = device;
				break;
			}
		}

		if (m_physical_device == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}

		vkGetPhysicalDeviceProperties(m_physical_device, &properties);
		// std::cout << "physical device: " << properties.deviceName << std::endl;
	}

	void JvscRenderer::create_device()
	{
		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queue_families = { m_graphics_family_index, m_present_family_index };

		float queue_priority = 1.0f;
		for (uint32_t queue_family : unique_queue_families) {
			VkDeviceQueueCreateInfo queue_info = {};
			queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_info.queueFamilyIndex = queue_family;
			queue_info.queueCount = 1;
			queue_info.pQueuePriorities = &queue_priority;
			queue_create_infos.push_back(queue_info);
		}

		VkPhysicalDeviceFeatures device_features = {};
		device_features.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo device_info = {};
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
		device_info.pQueueCreateInfos = queue_create_infos.data();

		device_info.pEnabledFeatures = &device_features;
		device_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
		device_info.ppEnabledExtensionNames = device_extensions.data();

		// deprecated
		if (enable_validation_layers) {
			device_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
			device_info.ppEnabledLayerNames = validation_layers.data();
		}
		else {
			device_info.enabledLayerCount = 0;
		}

		if (vkCreateDevice(m_physical_device, &device_info, nullptr, &m_device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(m_device, m_graphics_family_index, 0, &m_graphics_queue);
		vkGetDeviceQueue(m_device, m_present_family_index, 0, &m_present_queue);
	}

	void JvscRenderer::create_allocator()
	{
		VmaAllocatorCreateInfo allocator_info{};
		allocator_info.physicalDevice = m_physical_device;
		allocator_info.device = m_device;
		allocator_info.instance = m_instance;

		if (vmaCreateAllocator(&allocator_info, &m_allocator) != VK_SUCCESS)
			throw std::runtime_error("failed to create allocator");
	}

	void JvscRenderer::create_command_pool()
	{
		VkCommandPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.queueFamilyIndex = m_graphics_family_index;
		pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(m_device, &pool_info, nullptr, &m_command_pool) != VK_SUCCESS)
			throw std::runtime_error("failed to create command pool!");
	}

	void JvscRenderer::create_swapchain()
	{
		SwapChainSupportDetails swapchain_support = query_swapchain_support(m_physical_device);

		VkSurfaceFormatKHR surface_format = choose_surface_format(swapchain_support.formats);
		VkPresentModeKHR present_mode = choose_present_mode(swapchain_support.present_modes);
		VkExtent2D extent = choose_extent(swapchain_support.capabilities);

		uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
		if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount)
			image_count = swapchain_support.capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR swapchain_info = {};
		swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_info.surface = m_surface;

		swapchain_info.minImageCount = image_count;
		swapchain_info.imageFormat = surface_format.format;
		swapchain_info.imageColorSpace = surface_format.colorSpace;
		swapchain_info.imageExtent = extent;
		swapchain_info.imageArrayLayers = 1;
		swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queue_family_indices[] = { m_graphics_family_index, m_present_family_index};

		if (m_graphics_family_index != m_present_family_index)
		{
			swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchain_info.queueFamilyIndexCount = 2;
			swapchain_info.pQueueFamilyIndices = queue_family_indices;
		}
		else
		{
			swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		swapchain_info.preTransform = swapchain_support.capabilities.currentTransform;
		swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_info.presentMode = present_mode;
		swapchain_info.clipped = VK_TRUE;
		swapchain_info.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain;

		if (vkCreateSwapchainKHR(m_device, &swapchain_info, nullptr, &m_swapchain) != VK_SUCCESS)
			throw std::runtime_error("failed to create swap chain!");
	
		vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, nullptr);
		m_swapchain_images.resize(image_count);
		vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, m_swapchain_images.data());

		m_swapchain_image_format = surface_format.format;
		m_swapchain_extent = extent;
	}

	void JvscRenderer::create_swapchain_image_views()
	{
		m_swapchain_image_views.resize(m_swapchain_images.size());
		for (size_t i = 0; i < m_swapchain_images.size(); i++)
		{
			VkImageViewCreateInfo view_info{};
			view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view_info.image = m_swapchain_images[i];
			view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view_info.format = m_swapchain_image_format;
			view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view_info.subresourceRange.baseMipLevel = 0;
			view_info.subresourceRange.levelCount = 1;
			view_info.subresourceRange.baseArrayLayer = 0;
			view_info.subresourceRange.layerCount = 1;

			if(vkCreateImageView(m_device, &view_info, nullptr, &m_swapchain_image_views[i]) != VK_SUCCESS)
				throw std::runtime_error("failed to create image view!");
		}
	}

	void JvscRenderer::create_depth_resources()
	{

		m_depth_image_format = find_supported_format({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

		m_depth_images.resize(m_swapchain_images.size());
		m_depth_image_views.resize(m_swapchain_images.size());
		m_depth_image_allocations.resize(m_swapchain_images.size());

		for (int i = 0; i < m_depth_images.size(); i++)
		{
			VkImageCreateInfo image_info{};
			image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			image_info.imageType = VK_IMAGE_TYPE_2D;
			image_info.extent.width = m_swapchain_extent.width;
			image_info.extent.height = m_swapchain_extent.height;
			image_info.extent.depth = 1;
			image_info.mipLevels = 1;
			image_info.arrayLayers = 1;
			image_info.format = m_depth_image_format;
			image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
			image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			image_info.samples = VK_SAMPLE_COUNT_1_BIT;
			image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			image_info.flags = 0;

			VmaAllocationCreateInfo alloc_info = {};
			alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

			if (vmaCreateImage(m_allocator, &image_info, &alloc_info, &m_depth_images[i], &m_depth_image_allocations[i], nullptr) != VK_SUCCESS)
				throw std::runtime_error("failed to create depth image");

			VkImageViewCreateInfo view_info{};
			view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view_info.image = m_depth_images[i];
			view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view_info.format = m_depth_image_format;
			view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			view_info.subresourceRange.baseMipLevel = 0;
			view_info.subresourceRange.levelCount = 1;
			view_info.subresourceRange.baseArrayLayer = 0;
			view_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_device, &view_info, nullptr, &m_depth_image_views[i]) != VK_SUCCESS)
				throw std::runtime_error("failed to create depth image view");
		}
	}

	void JvscRenderer::create_render_pass()
	{
		VkAttachmentDescription color_attachment = {};
		color_attachment.format = m_swapchain_image_format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_ref = {};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depth_attachment{};
		depth_attachment.format = m_depth_image_format;
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_ref{};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstSubpass = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::vector<VkAttachmentDescription> attachments = { color_attachment, depth_attachment };
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_render_pass) != VK_SUCCESS)
			throw std::runtime_error("failed to create render pass!");
	}

	void JvscRenderer::create_framebuffers()
	{
		m_swapchain_framebuffers.resize(m_swapchain_images.size());
		for (size_t i = 0; i < m_swapchain_framebuffers.size(); i++) 
		{
			std::vector<VkImageView> attachments = { m_swapchain_image_views[i], m_depth_image_views[i] };

			VkFramebufferCreateInfo framebuffer_info = {};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = m_render_pass;
			framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebuffer_info.pAttachments = attachments.data();
			framebuffer_info.width = m_swapchain_extent.width;
			framebuffer_info.height = m_swapchain_extent.height;
			framebuffer_info.layers = 1;

			if (vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &m_swapchain_framebuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("failed to create framebuffer!");
		}

	}

	void JvscRenderer::create_sync_objects()
	{
		m_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
		m_images_in_flight.resize(m_swapchain_images.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphore_info = {};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_info = {};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_image_available_semaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_render_finished_semaphores[i]) != VK_SUCCESS ||
				vkCreateFence(m_device, &fence_info, nullptr, &m_in_flight_fences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}
	}

	void JvscRenderer::create_command_buffers()
	{
		m_command_buffers.resize(m_swapchain_images.size());

		VkCommandBufferAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandPool = m_command_pool;
		alloc_info.commandBufferCount = static_cast<uint32_t>(m_command_buffers.size());

		if (vkAllocateCommandBuffers(m_device, &alloc_info, m_command_buffers.data()) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate command buffers");
	}

	std::vector<const char*> JvscRenderer::get_required_extensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enable_validation_layers)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return extensions;
	}

	void JvscRenderer::glfw_required_extensions()
	{
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> extensions(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

		// std::cout << "available extensions:" << std::endl;
		std::unordered_set<std::string> available;
		for (const auto& extension : extensions) 
		{
			// std::cout << "\t" << extension.extensionName << std::endl;
			available.insert(extension.extensionName);
		}

		// std::cout << "required extensions:" << std::endl;
		auto requiredExtensions = get_required_extensions();
		for (const auto& required : requiredExtensions) 
		{
			// std::cout << "\t" << required << std::endl;
			if (available.find(required) == available.end())
				throw std::runtime_error("Missing required glfw extension");
		}
	}

	bool JvscRenderer::is_device_suitable(VkPhysicalDevice physical_device)
	{
		QueueFamilyIndices indices = find_queue_families(physical_device);

		bool extensions_supported = check_device_extension_support(physical_device);

		bool swapchain_adequate = false;
		if (extensions_supported) 
		{
			SwapChainSupportDetails swapchain_support = query_swapchain_support(physical_device);
			swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
		}

		VkPhysicalDeviceFeatures supported_features;
		vkGetPhysicalDeviceFeatures(physical_device, &supported_features);

		if (indices.is_complete() && extensions_supported && swapchain_adequate && supported_features.samplerAnisotropy)
		{
			m_graphics_family_index = indices.graphics_family_index;
			m_present_family_index = indices.present_family_index;
			return true;
		}
		return false;
	}

	bool JvscRenderer::check_device_extension_support(VkPhysicalDevice physical_device)
	{
		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());

		std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

		for (const auto& extension : available_extensions) {
			required_extensions.erase(extension.extensionName);
		}

		return required_extensions.empty();
	}

	QueueFamilyIndices JvscRenderer::find_queue_families(VkPhysicalDevice physical_device)
	{
		QueueFamilyIndices indices;

		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

		int i = 0;
		for (const auto& queue_family : queue_families) 
		{
			if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
			{
				indices.graphics_family_index = i;
				indices.graphics_family_has_value = true;
			}
			VkBool32 present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, m_surface, &present_support);
			if (queue_family.queueCount > 0 && present_support) 
			{
				indices.present_family_index = i;
				indices.present_family_has_value = true;
			}
			if (indices.is_complete())
				break;
			i++;
		}

		return indices;
	}

	VkFormat JvscRenderer::find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates) 
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
				return format;
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
				return format;
		}
		throw std::runtime_error("failed to find supported format!");
	}

	SwapChainSupportDetails JvscRenderer::query_swapchain_support(VkPhysicalDevice physical_device)
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, m_surface, &details.capabilities);

		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, m_surface, &format_count, nullptr);

		if (format_count != 0) {
			details.formats.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, m_surface, &format_count, details.formats.data());
		}

		uint32_t present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, m_surface, &present_mode_count, nullptr);

		if (present_mode_count != 0) {
			details.present_modes.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, m_surface, &present_mode_count, details.present_modes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR JvscRenderer::choose_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
	{
		for (const auto& available_format : available_formats) 
		{
			if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return available_format;
		}

		return available_formats[0];
	}

	VkPresentModeKHR JvscRenderer::choose_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
	{
		for (const auto& available_present_mode : available_present_modes) {
			if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) 
			{
				// std::cout << "Present mode: Mailbox" << std::endl;
				return available_present_mode;
			}
		}

		// std::cout << "Present mode: V-Sync" << std::endl;
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D JvscRenderer::choose_extent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;

		VkExtent2D actualExtent = m_window.get_extent();
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
		return actualExtent;
	}
}

