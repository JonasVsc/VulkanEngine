cmake_minimum_required(VERSION 3.28)

project(VulkanEngine 
	VERSION 0.0.1 
	DESCRIPTION "Vulkan Game Engines"
	LANGUAGES CXX)

set(VULKAN_SDK "C:\\VulkanSDK\\1.3.296.0")

add_subdirectory(external)
add_subdirectory(JvscEngine)