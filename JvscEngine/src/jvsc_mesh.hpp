#pragma once

#include "jvsc_renderer.hpp"

// lib
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <vector>

namespace jvsc {

	struct Vertex
	{
		glm::vec2 position;
		glm::vec3 color;

		static std::vector<VkVertexInputBindingDescription> get_binding_descriptions();
		static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions();
	};

	class JvscMesh
	{
	public:

		JvscMesh(JvscRenderer& renderer, std::vector<Vertex>& vertices);
		~JvscMesh() = default;
		void destroy();

		JvscMesh(const JvscMesh&) = delete;
		JvscMesh& operator=(const JvscMesh&) = delete;

		void bind(VkCommandBuffer cmd);
		void draw(VkCommandBuffer cmd);


	private:

		JvscRenderer& m_renderer;

		VkBuffer m_vertex_buffer;
		VmaAllocation m_vertex_buffer_allocation;
		uint32_t m_vertex_count;
	};

}