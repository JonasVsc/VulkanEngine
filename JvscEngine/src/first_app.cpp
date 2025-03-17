#include "first_app.hpp"

// lib
#include "systems/simple_render_system.hpp"

// std
#include <iostream>

jvsc::JvscMesh* create_cube_mesh(jvsc::JvscRenderer& renderer, glm::vec3 offset) {
    std::vector<jvsc::Vertex> vertices{

        // left face (white)
        {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
        {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
        {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
        {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
        {{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},
        {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},

        // right face (yellow)
        {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
        {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
        {{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
        {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
        {{.5f, .5f, -.5f}, {.8f, .8f, .1f}},
        {{.5f, .5f, .5f}, {.8f, .8f, .1f}},

        // top face (orange, remember y axis points down)
        {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
        {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
        {{-.5f, -.5f, .5f}, {.9f, .6f, .1f}},
        {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
        {{.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
        {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},

        // bottom face (red)
        {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
        {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
        {{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
        {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
        {{.5f, .5f, -.5f}, {.8f, .1f, .1f}},
        {{.5f, .5f, .5f}, {.8f, .1f, .1f}},

        // nose face (blue)
        {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
        {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
        {{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
        {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
        {{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
        {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},

        // tail face (green)
        {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
        {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
        {{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
        {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
        {{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
        {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
    };
    for (auto& v : vertices) {
        v.position += offset;
    }
    return new jvsc::JvscMesh(renderer, vertices);
}


FirstApp::FirstApp()
	: m_window{jvsc::JvscWindow::create_window(800, 600, "JvscEngine Cube example")}
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
    jvsc::JvscGameObject cube = jvsc::JvscGameObject::create_game_object();
    cube.mesh = create_cube_mesh(m_renderer, { 0.0f, 0.0f, 0.0f });
    cube.transform.translate = { .0f, .0f, .5f };
    cube.transform.scale = { .5f, .5f, .5f };
    m_game_objects.push_back(std::move(cube));
}
