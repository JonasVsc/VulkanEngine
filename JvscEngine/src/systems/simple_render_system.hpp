#pragma once

// lib
#include "jvsc_renderer.hpp"
#include "jvsc_pipeline.hpp"
#include "jvsc_game_object.hpp"

namespace jvsc {

	class SimpleRenderSystem
	{
	public:

		SimpleRenderSystem(JvscRenderer& renderer, VkRenderPass render_pass);
		~SimpleRenderSystem() = default;
		void terminate();

		void render_game_objects(VkCommandBuffer cmd, std::vector<JvscGameObject>& game_objects);

	private:
	
		void create_pipeline_layout();
		void create_pipeline(VkRenderPass render_pass);


		JvscRenderer& m_renderer;

		JvscPipeline* m_pipeline;
		VkPipelineLayout m_pipeline_layout;

	};

}