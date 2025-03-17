#pragma once

// lib
#include "jvsc_window.hpp"
#include <vma/vk_mem_alloc.h>

// std
#include <vector>

namespace jvsc {

	struct SwapChainSupportDetails 
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	};

	struct QueueFamilyIndices 
	{
		uint32_t graphics_family_index;
		uint32_t present_family_index;
		bool graphics_family_has_value = false;
		bool present_family_has_value = false;
		bool is_complete() const { return graphics_family_has_value && present_family_has_value; }
	};

	class JvscRenderer
	{
	public:

		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	#ifdef NDEBUG
			const bool enable_validation_layers = false;
	#else
			const bool enable_validation_layers = true;
	#endif

		JvscRenderer(JvscWindow& window);
		~JvscRenderer() = default;
		void terminate();
		
		VkCommandBuffer begin_frame();
		void begin_swapchain_render_pass(VkCommandBuffer cmd);
		void end_frame(VkCommandBuffer cmd);
		void handle_minimize();

		// getters
		VkRenderPass render_pass() const { return m_render_pass; }
		VkFramebuffer framebuffer(int index) const { return m_swapchain_framebuffers[index]; }
		VkExtent2D extent() const { return m_swapchain_extent; }
		VkCommandBuffer command_buffer(int index) { return m_command_buffers[index]; }
		VkDevice device() const { return m_device; }
		VmaAllocator allocator() const { return m_allocator; }


		JvscRenderer(const JvscRenderer&) = delete;
		JvscRenderer& operator=(const JvscRenderer&) = delete;
		JvscRenderer(JvscRenderer&&) = delete;
		JvscRenderer& operator=(JvscRenderer&&) = delete;

	private:

		void create_instance();
		void create_surface();
		void select_physical_device();
		void create_device();
		void create_allocator();
		void create_swapchain();
		void create_swapchain_image_views();
		void create_depth_resources();
		void create_render_pass();
		void create_framebuffers();
		void create_sync_objects();
		void create_command_pool();
		void create_command_buffers();

		bool is_device_suitable(VkPhysicalDevice device);
		bool check_device_extension_support(VkPhysicalDevice physical_device);
		std::vector<const char*> get_required_extensions();
		void glfw_required_extensions();
		QueueFamilyIndices find_queue_families(VkPhysicalDevice physical_device);
		VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice physical_device);
		VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
		VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes);
		VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR& capabilities);
		VkResult submit_command_buffers(uint32_t* image_index);

		JvscWindow& m_window;
		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debug_messenger;
		VkSurfaceKHR m_surface;
		VkPhysicalDevice m_physical_device;
		VkPhysicalDeviceProperties properties;
		VkDevice m_device;
		uint32_t m_graphics_family_index;
		VkQueue m_graphics_queue;
		uint32_t m_present_family_index;
		VkQueue m_present_queue;
		VmaAllocator m_allocator;
		VkCommandPool m_command_pool;
		VkSwapchainKHR m_swapchain;
		VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE;
		std::vector<VkImage> m_swapchain_images;
		std::vector<VkImageView> m_swapchain_image_views;
		VkFormat m_swapchain_image_format;
		VkExtent2D m_swapchain_extent;
		std::vector<VkImage> m_depth_images;
		std::vector<VmaAllocation> m_depth_image_allocations;
		std::vector<VkImageView> m_depth_image_views;
		VkFormat m_depth_image_format;
		VkRenderPass m_render_pass;
		std::vector<VkFramebuffer> m_swapchain_framebuffers;
		std::vector<VkSemaphore> m_image_available_semaphores;
		std::vector<VkSemaphore> m_render_finished_semaphores;
		std::vector<VkFence> m_in_flight_fences;
		std::vector<VkFence> m_images_in_flight;
		std::vector<VkCommandBuffer> m_command_buffers;
		uint32_t m_current_frame = 0;
		uint32_t m_image_index = 0;

		const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };
		const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	};

}