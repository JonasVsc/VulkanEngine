set(CMAKE_CXX_STANDARD 20)

add_executable(jvsc_engine src/main.cpp
	src/first_app.hpp
	src/first_app.cpp
	
	src/jvsc_window.hpp
	src/jvsc_window.cpp 

	src/jvsc_renderer.hpp
	src/jvsc_renderer.cpp

	src/jvsc_pipeline.hpp
	src/jvsc_pipeline.cpp
	
	src/jvsc_mesh.hpp
	src/jvsc_mesh.cpp

	src/jvsc_game_object.hpp

	# systems
	src/systems/simple_render_system.hpp
	src/systems/simple_render_system.cpp

)

target_link_libraries(jvsc_engine glfw ${VULKAN_SDK}/Lib/vulkan-1.lib)
target_include_directories(jvsc_engine PRIVATE glfw ${VULKAN_SDK}/Include src)