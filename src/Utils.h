#pragma once

#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

const int MAX_FRAME_DRAWS = 2;
const int MAX_OBJECTS = 20;

static const std::vector<const char*> s_DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Color;
	glm::vec2 TextureCoords; // (u,v)

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

static VkCommandBuffer BeginCommandBuffer(VkDevice device, VkCommandPool commandPool)
{
	// Command buffer to hold transfer commands
	VkCommandBuffer commandBuffer;

	// Command buffer details
	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandPool = commandPool;
	allocateInfo.commandBufferCount = 1;

	// Allocate command buffer from pool
	vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);

	// Info to begin the command buffer record
	VkCommandBufferBeginInfo cmdBeginInfo = {};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We`re only using the command buffer once, so set up for  


	// Begin recording transfer commands
	vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo);

	return commandBuffer;
}

static void FinishAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
{
	// End command
	vkEndCommandBuffer(commandBuffer);


	// Queue submission info
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Submit transfer command to transfer queue and wait until it finishes
	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	// Free temporary command buffer back to pool
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

}

static void CopyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCmdPool,
	VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
	// Create buffer
	VkCommandBuffer transferCommandBuffer = BeginCommandBuffer(device, transferCmdPool);
	
	// Region of data to copy from and to
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = 0;
	bufferCopyRegion.size = bufferSize;

	// Command to copy src buffer to dst buffer
	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	// End and submit command buffer
	FinishAndSubmitCommandBuffer(device, transferCmdPool, transferQueue, transferCommandBuffer);
}

static void CopyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
	VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height)
{
	// Create buffer
	VkCommandBuffer transferCommandBuffer = BeginCommandBuffer(device, transferCommandPool);

	VkBufferImageCopy imageRegion = {};
	imageRegion.bufferOffset = 0;											// Offset into data
	imageRegion.bufferRowLength = 0;										// row length of data to calculate data spacing
	imageRegion.bufferImageHeight = 0;										// image height to calculate data spacing
	imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// which aspect of image to copy
	imageRegion.imageSubresource.mipLevel = 0;								// Mipmap level to copy
	imageRegion.imageSubresource.baseArrayLayer = 0;						// Starting array layer
	imageRegion.imageSubresource.layerCount = 1;							// Number of layer to copy starting at baseArrayLayer
	imageRegion.imageOffset = { 0, 0, 0 };									// Offset into image (as opposed to raw data offset)
	imageRegion.imageExtent = { width, height, 1 };							// Size of region to copy as (x, y, z) values


	// Command to copy src buffer to dst image buffer
	vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

	// End and Submit command buffer
	FinishAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
}


static void TransitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	// Create buffer
	VkCommandBuffer commandBuffer = BeginCommandBuffer(device, commandPool);

	// Create
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout = oldLayout;									// Layout to transition from
	imageMemoryBarrier.newLayout = newLayout;									// Layout to transition to
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
	imageMemoryBarrier.image = image;											// image being accessed and modified as part of barrier
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;						// First mip level to start alterations on
	imageMemoryBarrier.subresourceRange.levelCount = 1;							// number of mip leves to alter starting from baseMipLevel
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;						// first layer to start alterations on
	imageMemoryBarrier.subresourceRange.layerCount = 1;							// number of layers to alter starting from baseArrayLayer
	

	VkPipelineStageFlags srcStage = 0;
	VkPipelineStageFlags dstStage = 0;


	// If transitioning from new image to image ready to receive data...
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = 0;							// Memory access stage transition must happen after...
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;	// Memory access stage transition must happen before...

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	// If transitioning from transfer destination to shader readable...
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	
	vkCmdPipelineBarrier(
		commandBuffer,
		srcStage, dstStage,					// Pipeline stages (match to sec and dst acessmask)
		0,						// Dependencies flags
		0, nullptr,				// memory barrier count + data
		0, nullptr,				// buffer memory barrier count + data
		1, &imageMemoryBarrier // Image memory barrier count + data
	);

	// End and Submit command buffer
	FinishAndSubmitCommandBuffer(device, commandPool, queue, commandBuffer);
}