#pragma once

#include "vk_camera.h"
#include "vk_mesh.h"

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

const int FRAME_OVERLAP = 2;

#define VK_CHECK(x)                                                        \
do 																           \
{                                                                          \
	VkResult err = x;                                                      \
	if (err) {                                                             \
			printf("Detected Vulkan error: %s", string_VkResult(err));     \
		abort();                                                           \
	}                                                                      \
} while (0)

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void add(std::function<void()>&& function)
	{
		deletors.push_back(function);
	}

	void flush()
	{
		for (auto it = deletors.rbegin(); it != deletors.rend(); ++it)
		{
			(*it)();
		}
	}
};

struct GPUCameraData
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
};

struct FrameData
{
	VkSemaphore present_semaphore, render_semaphore;
	VkFence render_fence;

	VkCommandPool command_pool;
	VkCommandBuffer main_command_buffer;

	AllocatedBuffer camera_buffer;
	VkDescriptorSet global_descriptor;

	AllocatedBuffer object_buffer;
	VkDescriptorSet object_descriptor;
};

struct MeshPushConstants
{
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct Material
{
	VkPipeline pipeline;
	VkPipelineLayout pipeline_layout;
};

struct RenderObject
{
	Mesh* mesh;
	Material* material;
	glm::mat4 transform_matrix;
};

class Renderer 
{
public:

	void init();

	void run();

	void shutdown();

private:

	void init_window();

	void init_vulkan();

	void init_instance(std::vector<const char*>& extensions, std::vector<const char*>& layers);
	
		// Used in instance initialization
		bool check_instance_extensions_support(std::vector<const char*>& required_extensions);
		bool check_layers_support(std::vector<const char*>& required_layers);
		std::vector<const char*> get_required_vulkan_extensions();

	void create_surface();

	void init_device(std::vector<const char*>& device_extensions, VkPhysicalDeviceFeatures enabled_features);
		
		// Used in device initialization
		void select_physical_device();
		bool check_device_extensions_support(std::vector<const char*>& required_device_extensions);

	void init_allocator();

	void create_swapchain();
		
		// Used in swapchain creation
		VkSurfaceCapabilitiesKHR get_surface_capabilities();
		std::vector<VkSurfaceFormatKHR> get_surface_formats();
		std::vector<VkPresentModeKHR> get_present_modes();
		VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes);
		VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
		VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

	void create_image_views();

	void create_default_renderpass();

	void create_framebuffers();

	void init_commands();

	void init_sync_objects();

	void init_descriptors();
	
		// Used in descriptors initialization
		void create_descriptor_set_layout();
		void create_descriptor_pool();
		void alloc_descriptor_sets();

	void init_pipeline();
		
		VkShaderModule load_shader_module(const char* file_path);

	void init_scene();

	void draw();

	AllocatedBuffer create_buffer(size_t buffer_size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags memory_flags = 0);

	inline FrameData& get_current_frame() { return m_frames[m_frame_number % FRAME_OVERLAP]; }

	void delta_time();

private:

	// Application
	bool m_running = true;
	uint32_t m_frame_number = 0;
	float m_delta_time;
	float m_last_frame;


	// Window
	const int m_window_width = 1280;
	const int m_window_height = 720;
	VkExtent2D m_window_extent;
	SDL_Window* m_window;

	// Vulkan
	VkInstance m_instance;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_gpu;
	VkPhysicalDeviceProperties m_gpu_properties;
	uint32_t m_graphics_family_index;
	VkQueue m_graphics_queue;
	VkDevice m_device;
	VmaAllocator m_allocator;
	VkSwapchainKHR m_swapchain;
	VkFormat m_swapchain_format;
	std::vector<VkImage> m_swapchain_images;
	std::vector<VkImageView> m_swapchain_image_views;
	VkRenderPass m_renderpass;
	std::vector<VkFramebuffer> m_framebuffers;
	FrameData m_frames[FRAME_OVERLAP];
	VkDescriptorSetLayout m_global_set_layout;
	VkDescriptorSetLayout m_object_set_layout;
	VkDescriptorPool m_descriptor_pool;
	VkPipeline m_pipeline;
	VkPipelineLayout m_pipeline_layout;

	// Scene
	std::vector<RenderObject> m_renderables;
	std::unordered_map<std::string, Material> m_materials;
	std::unordered_map<std::string, Mesh> m_meshes;

	Camera m_main_camera;
	AllocatedBuffer m_triangle_buffer;

	// Deletors
	DeletionQueue m_main_deletor;
};