#include "jvsc_mesh.hpp"


namespace jvsc {

	JvscMesh::JvscMesh(JvscRenderer& renderer, std::vector<Vertex>& vertices)
		: m_renderer{renderer}
	{
		// create buffer
		m_vertex_count = static_cast<uint32_t>(vertices.size());
		assert(m_vertex_count >= 3 && "vertex count must be at least 3");
		VkDeviceSize buffer_size = sizeof(vertices[0]) * m_vertex_count;

		VkBufferCreateInfo buffer_info{};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = buffer_size;
		buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo alloc_info{};
		alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
		alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

		if (vmaCreateBuffer(m_renderer.allocator(), &buffer_info, &alloc_info, &m_vertex_buffer, &m_vertex_buffer_allocation, nullptr) != VK_SUCCESS)
			throw std::runtime_error("failed to create buffer");

		void* data;
		vmaMapMemory(m_renderer.allocator(), m_vertex_buffer_allocation, &data);
			memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
		vmaUnmapMemory(m_renderer.allocator(), m_vertex_buffer_allocation);
	}

	void JvscMesh::destroy()
	{
		vmaDestroyBuffer(m_renderer.allocator(), m_vertex_buffer, m_vertex_buffer_allocation);
	}

	void JvscMesh::bind(VkCommandBuffer cmd)
	{
		VkBuffer buffers[] = { m_vertex_buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
	}

	void JvscMesh::draw(VkCommandBuffer cmd)
	{
		vkCmdDraw(cmd, m_vertex_count, 1, 0, 0);
	}

	std::vector<VkVertexInputBindingDescription> Vertex::get_binding_descriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindings(1);

		bindings[0].binding = 0;
		bindings[0].stride = sizeof(Vertex);
		bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindings;
	}

	std::vector<VkVertexInputAttributeDescription> Vertex::get_attribute_descriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributes(2);

		attributes[0].binding = 0;
		attributes[0].location = 0;
		attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributes[0].offset = offsetof(Vertex, position);

		attributes[1].binding = 0;
		attributes[1].location = 1;
		attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributes[1].offset = offsetof(Vertex, color);

		return attributes;
	}

}