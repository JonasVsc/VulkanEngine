#pragma once

#include "jvsc_window.hpp"
#include "jvsc_renderer.hpp"
#include "jvsc_pipeline.hpp"
#include "jvsc_game_object.hpp"

class FirstApp
{
public:

	FirstApp();

	~FirstApp();

	void run();

private:

	void load_game_objects();

	jvsc::JvscWindow& m_window;
	jvsc::JvscRenderer m_renderer{ m_window };
};