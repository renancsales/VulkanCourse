#pragma once

#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

const int MAX_FRAME_DRAWS = 2;
const int MAX_OBJECTS = 100;

static const std::vector<const char*> s_DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Color;

};

// Indices (locations) of queue families (if they exist at all)
struct QueueFamilyIndices
{
	int GraphicsFamily = -1; // location of graphics queue family
	int PresentationFamily = -1; // location of presentation queue family

	bool IsValid()
	{
		return GraphicsFamily >= 0 && PresentationFamily >=0;
	}
};

struct SwapChainDetails
{
	VkSurfaceCapabilitiesKHR SurfaceCapabilities; // Surface properties, e.g, image size/extent
	std::vector<VkSurfaceFormatKHR> Formats;	// Surface image formats, e.g, RGBA8
	std::vector<VkPresentModeKHR> PresentationMode; // How images should be presented to screen
};

struct SwapChainImage
{
	VkImage Image;
	VkImageView ImageView;
};


static std::vector<char> readSPVFile(const std::string& filename)
{
	//open file
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	// Check 
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open a file in readSPVFile");
	}

	// get size and create vector
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> fileBuffer(fileSize);

	// move read position to the start of the file
	file.seekg(0);

	// Read the file data into the bufer
	file.read(fileBuffer.data(), fileSize);

	file.close();

	return fileBuffer;
}

static uint32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags propertyFlags)
{
	// Get properties of physical device memory
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((allowedTypes & (1 << i))  // index of memory type must match corresponding bit in allowedTypes
			&& (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) // desire property bit flags are part of memory type`s property flags
		{
			// This memory type is valid
			return i;
		}
	}

	return 0;
}

static void CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags,
	VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
	// Info to create a buffer (it doesnt include assigning memory)
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = bufferSize;  // Size of buffer
	bufferCreateInfo.usage = bufferUsageFlags; // multiple type of buffer possible 
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// Similat to swapchain images, it can share vertex buffer

	VkResult result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a vertex buffer!");
	}

	// Get buffer memory requirements
	VkMemoryRequirements memRequirements = {};
	vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

	// Allocate memory to buffer
	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = FindMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, // index of memory type on physical device that has required bit flags
		bufferProperties);																	// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : CPU can interacte with memory
																							// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: Allow placement of data straight into buffer after mapping (otherwise it would have to specify manually)
	// Allocate memory to VkDeviceMemory
	result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, bufferMemory);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate vertex buffer memory!");
	}

	// Allocate memory to given vertex buffer
	vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

static void CopyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCmdPool,
	VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
	// Command buffer to hold transfer commands
	VkCommandBuffer transferCommandBuffer;

	// Command buffer details
	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandPool = transferCmdPool;
	allocateInfo.commandBufferCount = 1;

	// Allocate command buffer from pool
	vkAllocateCommandBuffers(device, &allocateInfo, &transferCommandBuffer);

	// Info to begin the command buffer record
	VkCommandBufferBeginInfo cmdBeginInfo = {};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We`re only using the command buffer once, so set up for  


	// Begin recording transfer commands
	vkBeginCommandBuffer(transferCommandBuffer, &cmdBeginInfo);

	// Region of data to copy from and to
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = 0;
	bufferCopyRegion.size = bufferSize;

	// Command to copy src buffer to dst buffer
	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	// end command
	vkEndCommandBuffer(transferCommandBuffer);


	// Queue submission info
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;

	// Submit transfer command to transfer queue and wait until it finishes
	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueue);

	// Free temporary command buffer back to pool
	vkFreeCommandBuffers(device, transferCmdPool, 1, &transferCommandBuffer);
}