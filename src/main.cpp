#define STB_IMAGE_IMPLEMENTATION
#define GLFW_FORCE_DEPTH_ZERO_TO_ONE

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


	float angle = 0.0f;
	float deltaTime = 0.0f;
	float lastTime = 0.0f;

	glm::mat4 modelMatrix;
	// lopp until closed
	while (!glfwWindowShouldClose(g_Window))
	{
		glfwPollEvents();
		
		float currentTime = (float)glfwGetTime();
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		angle += 40 * deltaTime;
		if (angle > 360.0f)
			angle -= 360.f;

		modelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angle), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { 1.5f, 1.5f, 1.5f });
		g_VulkanRenderer.UpdateModel(1, modelMatrix);
		modelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-angle), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { 1.5f, 1.5f, 1.5f });
		g_VulkanRenderer.UpdateModel(0, modelMatrix);

		g_VulkanRenderer.Draw();
		//std::cout << "Delta time: " << deltaTime << "s" << "  / FPS: " << 1.0 / deltaTime << std::endl;
	}

	

	// destroy glfw window and stop glfw
	glfwDestroyWindow(g_Window);
	glfwTerminate();

	g_VulkanRenderer.CleanUp();

	return 0;
}