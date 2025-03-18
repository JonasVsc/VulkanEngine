#include "first_app.hpp"

#include "systems/simple_render_system.hpp"
#include "systems/gravity_physics_system.hpp"
#include "systems/vec2_field_system.hpp"

// lib
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <iostream>
#include <random>

jvsc::JvscMesh* createSquareModel(jvsc::JvscRenderer& renderer, glm::vec2 offset) 
{
	std::vector<jvsc::Vertex> vertices = {
		{{-0.5f, -0.5f}},
		{{0.5f, 0.5f}},
		{{-0.5f, 0.5f}},
		{{-0.5f, -0.5f}},
		{{0.5f, -0.5f}},
		{{0.5f, 0.5f}},
	};
	for (auto& v : vertices) {
		v.position += offset;
	}
	return new jvsc::JvscMesh(renderer, vertices);
}

jvsc::JvscMesh* createCircleModel(jvsc::JvscRenderer& renderer, unsigned int numSides) 
{
	std::vector<jvsc::Vertex> uniqueVertices{};
	for (int i = 0; i < numSides; i++) 
	{
		float angle = i * glm::two_pi<float>() / numSides;
		uniqueVertices.push_back({ {glm::cos(angle), glm::sin(angle)} });
	}
	uniqueVertices.push_back({});  // adds center vertex at 0, 0

	std::vector<jvsc::Vertex> vertices{};
	for (int i = 0; i < numSides; i++) 
	{
		vertices.push_back(uniqueVertices[i]);
		vertices.push_back(uniqueVertices[(i + 1) % numSides]);
		vertices.push_back(uniqueVertices[numSides]);
	}
	return new jvsc::JvscMesh(renderer, vertices);
}


FirstApp::FirstApp()
	: m_window{jvsc::JvscWindow::create_window(1000, 800, "JvscEngine physics example")}
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
	// create some models
	jvsc::JvscMesh* circleMesh = createCircleModel(m_renderer, 64);

	// create physics objects
	std::vector<jvsc::JvscGameObject> physicsObjects{};
	std::random_device rd;  // Dispositivo de geração de números aleatórios
	std::mt19937 gen(rd()); // Motor Mersenne Twister
	std::uniform_real_distribution<float> dist(-1.f, 1.f); // Define o intervalo
	std::uniform_real_distribution<float> vel(-0.1f, 0.1f); // Define o intervalo
	
	int min = -2;
	int max = 2;

	for (int i = min; i < max; i++)
	{
		for (int j = min; j < max; j++)
		{
			auto circle = jvsc::JvscGameObject::create_game_object();
			circle.transform.scale = glm::vec2{ .01f };
			circle.transform.translation = { dist(gen), dist(gen) };
			circle.mesh = circleMesh;
			circle.color = { dist(gen), dist(gen), dist(gen) };
			circle.rigidbody2d.velocity = { vel(gen), vel(gen)};
			physicsObjects.push_back(std::move(circle));
		}
	}

	jvsc::GravityPhysicsSystem gravitySystem{ 0.001f };
	jvsc::Vec2FieldSystem vecFieldSystem{};
	jvsc::SimpleRenderSystem simpleRenderSystem{ m_renderer, m_renderer.render_pass() };

	while (!m_window.should_close())
	{
		glfwPollEvents();
		gravitySystem.update(physicsObjects, 1.f / 60, 5);
		VkCommandBuffer cmd = m_renderer.begin_frame();

		
		m_renderer.begin_swapchain_render_pass(cmd);

		simpleRenderSystem.render_game_objects(cmd, physicsObjects);

		vkCmdEndRenderPass(cmd);
		m_renderer.end_frame(cmd);
	}

	vkDeviceWaitIdle(m_renderer.device());

	circleMesh->destroy();
	simpleRenderSystem.terminate();
}

void FirstApp::load_game_objects()
{
	
}
