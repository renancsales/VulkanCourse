#pragma once


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

	void UpdateModel(uint32_t meshObjectIndex, glm::mat4& newModel);

	void Draw();
	void CleanUp();

private:
	// Create functions
	void CreateInstance();
	void CreateLogicalDevice();
	void CreateSurface();
	void CreateSwapChain();
	void CreateRenderPass();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateDepthBufferImage();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSynchronization();

	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	void UpdateUniformBuffers(uint32_t imageIndex);

	// Recodrd functions
	void RecordCommands(uint32_t currentImageIndex);

	// Get functions
	void GetPhysicalDevice();

	// - Allocate functions
	void AllocateDynamicBufferTransferSpace();

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
	VkFormat ChooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	// validation layer
	bool CheckValidationLayerSupport();

	// -- Create functions
	VkImage CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory);
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	
private:
	GLFWwindow* m_Window;

	int m_CurrentFrame = 0;

	// Scene objects
	std::vector<Mesh> m_MeshList;
	// Scene settings
	struct Camera {
		glm::mat4 Projection;
		glm::mat4 View;

	} m_Camera;

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

	VkImage m_DepthBufferImage;
	VkFormat m_DepthBufferFormat;
	VkDeviceMemory m_DepthBufferImageMemory;
	VkImageView m_DepthBufferImageView;

	// - Descriptors
	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkDescriptorPool m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;

	std::vector<VkBuffer> m_UniformBuffers;
	std::vector<VkDeviceMemory> m_UniformBufferMemory;

	std::vector<VkBuffer> m_UniformDynamicBuffers;
	std::vector<VkDeviceMemory> m_UniformDynamicBufferMemory;

	VkDeviceSize m_MinUniformBufferOffset;
	size_t m_ModelUniformAlignment;
	UniformBufferObjectModel* m_ModelTransferSpace = nullptr;

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