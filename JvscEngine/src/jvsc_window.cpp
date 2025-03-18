#include "jvsc_window.hpp"

// std
#include <string>
#include <stdexcept>
#include <iostream>

namespace jvsc {

	JvscWindow* JvscWindow::s_instance = nullptr;

	JvscWindow& JvscWindow::create_window(int width, int height, const std::string& name) 
	{
		if (!JvscWindow::s_instance)
			JvscWindow::s_instance = new JvscWindow(width, height, name);
		else
			throw std::runtime_error("only 1 window per application");
		return *JvscWindow::s_instance;
	}

	JvscWindow& JvscWindow::get_window() {
		if (!JvscWindow::s_instance)
			throw std::runtime_error("the window wasn't initialized before get");
		return *JvscWindow::s_instance;
	}

	JvscWindow::JvscWindow(int width, int height, const std::string& name)
		: m_width{ width }
		, m_height{ height }
		, m_name{ name }
	{
		std::cout << "calling m_window constructor" << '\n';
		init(width, height, name);
	}

	void JvscWindow::terminate()
	{
		std::cout << "calling m_window destructor" << '\n';
		glfwDestroyWindow(m_window);
		glfwTerminate();
		delete s_instance;
	}

	void JvscWindow::create_surface(VkInstance instance, VkSurfaceKHR* surface)
	{
		if (glfwCreateWindowSurface(instance, m_window, nullptr, surface) != VK_SUCCESS)
			throw std::runtime_error("failed to create window surface");
	}

	void JvscWindow::init(int width, int height, const std::string& name)
	{
		if (!glfwInit())
			throw std::runtime_error("glfw init failed");

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		int count = 0;
		GLFWmonitor** monitors = glfwGetMonitors(&count);

		m_window = glfwCreateWindow(width, height, name.c_str(), monitors[1], nullptr);

		if (!m_window)
			throw std::runtime_error("window creation failed");
		
		glfwSetWindowUserPointer(m_window, this);
	}
}