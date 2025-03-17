#include "first_app.hpp"


int main()
{
	FirstApp app;

	try
	{
		app.run();
	}
	catch (const std::exception&)
	{

	}

	return 0;
}