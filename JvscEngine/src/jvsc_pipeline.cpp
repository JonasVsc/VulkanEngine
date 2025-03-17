#include "jvsc_pipeline.hpp"

// lib
#include "jvsc_mesh.hpp"

// std
#include <cassert>
#include <fstream>
#include <iostream>


namespace jvsc {

	JvscPipeline::JvscPipeline(JvscRenderer& renderer, const std::string& vertex_filepath, const std::string& fragment_filepath, const PipelineBuilder& pipeline_builder)
		: m_renderer{renderer}
	{
		std::cout << "calling pipeline constructor" << '\n';
		create_graphics_pipeline(vertex_filepath, fragment_filepath, pipeline_builder);
	}

	void JvscPipeline::destroy()
	{
		std::cout << "calling pipeline destructor" << '\n';
		vkDestroyPipeline(m_renderer.device(), m_graphics_pipeline, nullptr);
	}


	void JvscPipeline::bind(VkCommandBuffer cmd)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);
	}

	void JvscPipeline::default_pipeline_builder(PipelineBuilder& pipeline_builder)
	{
		pipeline_builder.inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pipeline_builder.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipeline_builder.inputAssembly.primitiveRestartEnable = VK_FALSE;

		pipeline_builder.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		pipeline_builder.viewportInfo.viewportCount = 1;
		pipeline_builder.viewportInfo.pViewports = nullptr;
		pipeline_builder.viewportInfo.scissorCount = 1;
		pipeline_builder.viewportInfo.pScissors = nullptr;

		pipeline_builder.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		pipeline_builder.rasterizationInfo.depthClampEnable = VK_FALSE;
		pipeline_builder.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		pipeline_builder.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		pipeline_builder.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		pipeline_builder.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		pipeline_builder.rasterizationInfo.depthBiasEnable = VK_FALSE;
		pipeline_builder.rasterizationInfo.depthBiasConstantFactor = 0.0f;
		pipeline_builder.rasterizationInfo.depthBiasClamp = 0.0f;
		pipeline_builder.rasterizationInfo.depthBiasSlopeFactor = 0.0f;
		pipeline_builder.rasterizationInfo.lineWidth = 1.0f;

		pipeline_builder.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		pipeline_builder.multisampleInfo.sampleShadingEnable = VK_FALSE;
		pipeline_builder.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipeline_builder.multisampleInfo.minSampleShading = 1.0f;
		pipeline_builder.multisampleInfo.pSampleMask = nullptr;
		pipeline_builder.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
		pipeline_builder.multisampleInfo.alphaToOneEnable = VK_FALSE;

		pipeline_builder.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		pipeline_builder.colorBlendAttachment.blendEnable = VK_FALSE;
		pipeline_builder.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
		pipeline_builder.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		pipeline_builder.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
		pipeline_builder.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
		pipeline_builder.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		pipeline_builder.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

		pipeline_builder.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		pipeline_builder.colorBlendInfo.logicOpEnable = VK_FALSE;
		pipeline_builder.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
		pipeline_builder.colorBlendInfo.attachmentCount = 1;
		pipeline_builder.colorBlendInfo.pAttachments = &pipeline_builder.colorBlendAttachment;
		pipeline_builder.colorBlendInfo.blendConstants[0] = 0.0f; // Optional
		pipeline_builder.colorBlendInfo.blendConstants[1] = 0.0f; // Optional
		pipeline_builder.colorBlendInfo.blendConstants[2] = 0.0f; // Optional
		pipeline_builder.colorBlendInfo.blendConstants[3] = 0.0f; // Optional
		
		pipeline_builder.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		pipeline_builder.depthStencilInfo.depthTestEnable = VK_TRUE;
		pipeline_builder.depthStencilInfo.depthWriteEnable = VK_TRUE;
		pipeline_builder.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		pipeline_builder.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		pipeline_builder.depthStencilInfo.minDepthBounds = 0.0f; // Optional
		pipeline_builder.depthStencilInfo.maxDepthBounds = 1.0f; // Optional
		pipeline_builder.depthStencilInfo.front = {}; // Optional
		pipeline_builder.depthStencilInfo.back = {}; // Optional
		
		pipeline_builder.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		pipeline_builder.dynamicStateInfo.flags = 0;
	}

	void JvscPipeline::create_graphics_pipeline(const std::string& vertex_filepath, const std::string& fragment_filepath, const PipelineBuilder& pipeline_builder)
	{
		assert(pipeline_builder.pipelineLayout != VK_NULL_HANDLE && "Cannot create graphics pipeline: no VkPipelineLayout provided in configInfo");
		assert(pipeline_builder.renderPass != VK_NULL_HANDLE && "Cannot create graphics pipeline: no VkRenderPass provided in configInfo");

		auto vert_code = read_file(vertex_filepath);
		auto frag_code = read_file(fragment_filepath);

		create_shader_module(vert_code, &m_vert_shader_module);
		create_shader_module(frag_code, &m_frag_shader_module);

		VkPipelineShaderStageCreateInfo shader_stages[2];
		shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stages[0].module = m_vert_shader_module;
		shader_stages[0].pName = "main";
		shader_stages[0].flags = 0;
		shader_stages[0].pNext = nullptr;
		shader_stages[0].pSpecializationInfo = nullptr;
	
		shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stages[1].module = m_frag_shader_module;
		shader_stages[1].pName = "main";
		shader_stages[1].flags = 0;
		shader_stages[1].pNext = nullptr;
		shader_stages[1].pSpecializationInfo = nullptr;

		auto binding_descriptions = Vertex::get_binding_descriptions();
		auto attribute_descriptions = Vertex::get_attribute_descriptions();
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(binding_descriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = binding_descriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attribute_descriptions.data();

		VkGraphicsPipelineCreateInfo pipeline_info{};
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = 2;
		pipeline_info.pStages = shader_stages;
		pipeline_info.pVertexInputState = &vertexInputInfo;
		pipeline_info.pInputAssemblyState = &pipeline_builder.inputAssembly;
		pipeline_info.pViewportState = &pipeline_builder.viewportInfo;
		pipeline_info.pRasterizationState = &pipeline_builder.rasterizationInfo;
		pipeline_info.pMultisampleState = &pipeline_builder.multisampleInfo;
		pipeline_info.pColorBlendState = &pipeline_builder.colorBlendInfo;
		pipeline_info.pDepthStencilState = &pipeline_builder.depthStencilInfo;
		pipeline_info.pDynamicState = &pipeline_builder.dynamicStateInfo;
	
		pipeline_info.layout = pipeline_builder.pipelineLayout;
		pipeline_info.renderPass = pipeline_builder.renderPass;
		pipeline_info.subpass = pipeline_builder.subpass;

		pipeline_info.basePipelineIndex = -1;
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(m_renderer.device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_graphics_pipeline) != VK_SUCCESS)
			throw std::runtime_error("failed to create graphics pipeline");

		vkDestroyShaderModule(m_renderer.device(), m_vert_shader_module, nullptr);
		vkDestroyShaderModule(m_renderer.device(), m_frag_shader_module, nullptr);
	}

	void JvscPipeline::create_shader_module(const std::vector<char>& code, VkShaderModule* shader_module)
	{
		VkShaderModuleCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = code.size();
		create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

		if (vkCreateShaderModule(m_renderer.device(), &create_info, nullptr, shader_module) != VK_SUCCESS)
			throw std::runtime_error("failed to create shader module");
	}

	std::vector<char> JvscPipeline::read_file(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			throw std::runtime_error("failed to open file: " + filepath);

		size_t file_size = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(file_size);

		file.seekg(0);
		file.read(buffer.data(), file_size);

		file.close();
		return buffer;
	}


}
