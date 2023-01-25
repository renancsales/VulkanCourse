#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Utils.h"

class Mesh
{
public:
	Mesh() = default;
	Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<Vertex>* vertices);

	~Mesh();

	int GetVertexCount();
	VkBuffer GetVertexBuffer();

	void DestroyVertexBuffer();


private:
	void CreateVertexBuffer(std::vector<Vertex>* vertices);
	uint32_t FindMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags propertyFlags);

private:
	size_t m_VertexCount;
	VkBuffer m_VertexBuffer = nullptr;
	VkDeviceMemory m_VertexBufferMemory;

	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_Device;
};

