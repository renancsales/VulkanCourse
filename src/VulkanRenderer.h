#pragma once


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
// Check only the generic errors
#include <stdexcept>
#include <vector>
#include <set>

#include "Utils.h"


// Enable validation layers only in debug mode
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class VulkanRenderer
{
public:
	VulkanRenderer() = default;
	~VulkanRenderer() = default;

	struct Devices
	{
		VkPhysicalDevice PhysicalDevice;
		VkDevice LogicalDevice;
	};

public:
	int Init(GLFWwindow* window);

	void CleanUp();

private:
	// Create functions
	void CreateInstance();
	void CreateLogicalDevice();
	void CreateSurface();

	// Get functions
	void GetPhysicalDevice();

	// Support functions
	// -- Check functions
	bool CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool CheckDeviceSuitable(VkPhysicalDevice device);
	// -- getter functions
	QueueFamilyIndices& GetQueueFamilies(VkPhysicalDevice device);


	// validatation layer
	bool CheckValidationLayerSupport();

private:
	GLFWwindow* m_Window;

	// vulkan components
	VkInstance m_Instance;
	Devices m_MainDevice;
	VkQueue m_GraphicsQueue;
	VkQueue m_PresentationQueue;

	VkSurfaceKHR m_Surface;
};