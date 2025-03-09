cmake_minimum_required(VERSION 3.28)
project(VulkanEngine)

set(VULKAN_SDK "C:\\VulkanSDK\\1.3.296.0")
set(CMAKE_CXX_STANDARD 20)

add_executable(vulkan_engine "src/main.cpp" 
	"src/vk_renderer.h" 
	"src/vk_renderer.cpp"

	"src/vk_mesh.h"
	"src/vk_mesh.cpp"

	"src/tiny_obj_loader.h"
	
	"src/vk_camera.h" 
	"src/vk_camera.cpp"
)

target_precompile_headers(vulkan_engine PRIVATE "src/pch.h")

target_include_directories(vulkan_engine PRIVATE ${VULKAN_SDK}/Include)

target_link_libraries(vulkan_engine PRIVATE 
	${VULKAN_SDK}/Lib/vulkan-1.lib 
	${VULKAN_SDK}/Lib/SDL2main.lib
	${VULKAN_SDK}/Lib/SDL2.lib 
)

file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})