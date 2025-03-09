#include<iostream>

#include"vk_renderer.h"

int main(int argc, char* argv[])
{
	try
	{
		Renderer renderer;

		renderer.init();

		renderer.run();

	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}