#pragma once

// lib
#include "jvsc_renderer.hpp"

// std
#include <string>
#include <vector>

namespace jvsc {

	struct PipelineBuilder
	{
		PipelineBuilder& operator=(const PipelineBuilder&) = delete;

		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssembly;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;
	};

	class JvscPipeline
	{
	public:

		JvscPipeline(JvscRenderer& renderer, const std::string& vertex_filepath, const std::string& fragment_filepath, const PipelineBuilder& pipeline_builder);
		~JvscPipeline() = default;
		void destroy();

		JvscPipeline(const JvscPipeline&) = delete;
		JvscPipeline& operator=(JvscPipeline&) = delete;

		void bind(VkCommandBuffer cmd);

		static void default_pipeline_builder(PipelineBuilder& pipeline_builder);

	private:

		void create_graphics_pipeline(const std::string& vertex_filepath, const std::string& fragment_filepath, const PipelineBuilder& pipeline_builder);

		void create_shader_module(const std::vector<char>& code, VkShaderModule* shader_module);

		static std::vector<char> read_file(const std::string& filepath);

		JvscRenderer& m_renderer;

		VkPipeline m_graphics_pipeline;
		VkShaderModule m_vert_shader_module;
		VkShaderModule m_frag_shader_module;

	};

}