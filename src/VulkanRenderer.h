#pragma once


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
// Check only the generic errors
#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>
#include <array>

#include "Mesh.h"
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
	void Draw();
	void CleanUp();

private:
	// Create functions
	void CreateInstance();
	void CreateLogicalDevice();
	void CreateSurface();
	void CreateSwapChain();
	void CreateRenderPass();
	void CreateGraphicsPipeline();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSynchronization();

	// Recodrd functions
	void RecordCommands();

	// Get functions
	void GetPhysicalDevice();

	// Support functions
	// -- Check functions
	bool CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	bool CheckDeviceSuitable(VkPhysicalDevice device);
	// -- getter functions
	QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device);
	SwapChainDetails GetSwapChainDetails(VkPhysicalDevice device);

	// -- pick functions
	VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR ChooseBestPresentationMode(const std::vector< VkPresentModeKHR>& presentationModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
	// validatation layer
	bool CheckValidationLayerSupport();

	// -- Create functions
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	
private:
	GLFWwindow* m_Window;

	int m_CurrentFrame = 0;

	// Scene objects
	Mesh m_MeshObject;

	// -- Vulkan components
	VkInstance m_Instance;
	Devices m_MainDevice;
	VkQueue m_GraphicsQueue;
	VkQueue m_PresentationQueue;

	VkSurfaceKHR m_Surface;
	VkSwapchainKHR m_Swapchain;

	std::vector<SwapChainImage> m_SwapChainImages;
	std::vector<VkFramebuffer> m_SwapChainFramebuffers;
	std::vector<VkCommandBuffer> m_CommandBuffers;

	// -- Pipeline
	VkPipeline m_GraphicsPipeline;
	VkPipelineLayout m_PipelineLayout;
	VkRenderPass m_RenderPass;

	// -- Pools
	VkCommandPool m_GraphicsCommandPool;

	// Utilities
	VkFormat m_SwapchainImageFormat;
	VkExtent2D m_SwapchainExtent;

	// - Synchronization
	std::vector<VkSemaphore> m_SemaphoresImageAvailable;
	std::vector<VkSemaphore> m_SemaphoresRenderFinished;
	std::vector<VkFence> m_DrawFences;
};