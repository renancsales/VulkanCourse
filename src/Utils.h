#pragma once

static const std::vector<const char*> s_DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
