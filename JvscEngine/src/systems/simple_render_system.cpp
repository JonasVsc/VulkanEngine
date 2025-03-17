#include "simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>


struct SimplePushConstantData {
	glm::mat2 transform{ 1.f };
	glm::vec2 offset;
	alignas(16) glm::vec3 color;
};

jvsc::SimpleRenderSystem::SimpleRenderSystem(JvscRenderer& renderer, VkRenderPass render_pass)
	: m_renderer{renderer}
{
	create_pipeline_layout();
	create_pipeline(render_pass);
}

void jvsc::SimpleRenderSystem::terminate()
{
	m_pipeline->destroy();
	vkDestroyPipelineLayout(m_renderer.device(), m_pipeline_layout, nullptr);
}

void jvsc::SimpleRenderSystem::render_game_objects(VkCommandBuffer cmd, std::vector<JvscGameObject>& game_objects)
{
	m_pipeline->bind(cmd);

	for (auto& obj : game_objects)
	{
		SimplePushConstantData push;
		push.offset = obj.transform.translation;
		push.color = obj.color;
		push.transform = obj.transform.mat2();

		vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
		
		obj.mesh->bind(cmd);
		obj.mesh->draw(cmd);
	}
}

void jvsc::SimpleRenderSystem::create_pipeline_layout()
{
	VkPushConstantRange push_constant_range{};
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(SimplePushConstantData);

	VkPipelineLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.pushConstantRangeCount = 1;
	layout_info.pPushConstantRanges = &push_constant_range;

	if (vkCreatePipelineLayout(m_renderer.device(), &layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout");
}

void jvsc::SimpleRenderSystem::create_pipeline(VkRenderPass render_pass)
{
	jvsc::PipelineBuilder pipeline_builder{};
	jvsc::JvscPipeline::default_pipeline_builder(pipeline_builder);

	VkViewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_renderer.extent().width);
	viewport.height = static_cast<float>(m_renderer.extent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{ {0, 0}, m_renderer.extent() };

	pipeline_builder.viewportInfo.pViewports = &viewport;
	pipeline_builder.viewportInfo.pScissors = &scissor;
	pipeline_builder.renderPass = render_pass;
	pipeline_builder.pipelineLayout = m_pipeline_layout;

	m_pipeline = new jvsc::JvscPipeline(m_renderer, "simple_shader.vert.spv", "simple_shader.frag.spv", pipeline_builder);
}
