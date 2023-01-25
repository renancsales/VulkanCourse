#pragma once

#include <fstream>
#include <glm/glm.hpp>

const int MAX_FRAME_DRAWS = 2;

static const std::vector<const char*> s_DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


struct Vertex
{
	glm::vec3 Position;

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