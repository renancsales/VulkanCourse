#pragma once

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