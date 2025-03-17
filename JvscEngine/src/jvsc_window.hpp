#pragma once

// libs
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// std
#include <string>
#include <stdexcept>

namespace jvsc {

	class JvscWindow
	{
	public:

		static JvscWindow& create_window(int width, int height, const std::string& name);

		static JvscWindow& get_window();

		void terminate();

		GLFWwindow* handle() { return m_window; }
		bool should_close() { return glfwWindowShouldClose(m_window); };
		VkExtent2D get_extent() { return { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height) }; }
		void create_surface(VkInstance instance, VkSurfaceKHR* surface);


	private:

		JvscWindow(int width, int height, const std::string& name);
		~JvscWindow() = default;
		JvscWindow(const JvscWindow&) = delete;
		JvscWindow& operator=(const JvscWindow&) = delete;

		static JvscWindow* s_instance;

		void init(int width, int height, const std::string& name);

		GLFWwindow* m_window;

		int m_width, m_height;

		std::string m_name;
	};

}