#include "first_app.hpp"

// lib
#include "systems/simple_render_system.hpp"

// std
#include <iostream>


FirstApp::FirstApp()
	: m_window{jvsc::JvscWindow::create_window(800, 600, "Hello, Vulkan!")}
{
	load_game_objects();
}

FirstApp::~FirstApp()
{	
	m_renderer.terminate();
	m_window.terminate();
}

void FirstApp::run()
{
	jvsc::SimpleRenderSystem simple_render_system{ m_renderer, m_renderer.render_pass() };

	while (!m_window.should_close())
	{
		glfwPollEvents();
		VkCommandBuffer cmd = m_renderer.begin_frame();
		m_renderer.begin_swapchain_render_pass(cmd);

		simple_render_system.render_game_objects(cmd, m_game_objects);

		vkCmdEndRenderPass(cmd);
		m_renderer.end_frame(cmd);
	}

	vkDeviceWaitIdle(m_renderer.device());

	for (auto& obj : m_game_objects)
	{
		obj.destroy();
	}

	simple_render_system.terminate();
}

void FirstApp::load_game_objects()
{
	std::vector<jvsc::Vertex> vertices = {
		{ { 0.0f,-0.5f }, { 1.0f, 0.0f, 0.0f } },
		{ { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
		{ {-0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } } 
	};
	auto mesh = new jvsc::JvscMesh(m_renderer, vertices);

	auto monkey = jvsc::JvscGameObject::create_game_object();
	monkey.mesh = mesh;
	monkey.color = { 0.8f, 0.2f, 0.0f };
	
	m_game_objects.push_back(std::move(monkey));
}
