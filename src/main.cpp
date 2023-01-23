#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

#include "VulkanRenderer.h"

static GLFWwindow* g_Window; // global var
static VulkanRenderer g_VulkanRenderer;

void initWindow(std::string wname = "test window", const int width = 800, const int height = 600)
{
	// Initialize GLFW
	glfwInit();

	// Set GLFW to nor work with opengl
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	g_Window = glfwCreateWindow(width, height, wname.c_str(), nullptr, nullptr);
}

int main()
{
	// Create window
	initWindow("Main window", 800, 600);

	// Create vulkan renderer instance
	if (g_VulkanRenderer.Init(g_Window) == EXIT_FAILURE)
		return EXIT_FAILURE;


	// lopp until closed
	while (!glfwWindowShouldClose(g_Window))
	{
		glfwPollEvents();
		g_VulkanRenderer.Draw();
	}

	

	// destroy glfw window and stop glfw
	glfwDestroyWindow(g_Window);
	glfwTerminate();

	g_VulkanRenderer.CleanUp();

	return 0;
}