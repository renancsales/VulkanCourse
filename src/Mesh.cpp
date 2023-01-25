#include "Mesh.h"


Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<Vertex>* vertices)
{
	m_VertexCount = vertices->size();
	m_PhysicalDevice = newPhysicalDevice;
	m_Device = newDevice;
	CreateVertexBuffer(vertices);

}


Mesh::~Mesh()
{
}

int Mesh::GetVertexCount()
{
	return m_VertexCount;
}

VkBuffer Mesh::GetVertexBuffer()
{
	return m_VertexBuffer;
}

void Mesh::DestroyVertexBuffer()
{
	vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
	vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);
}

void Mesh::CreateVertexBuffer(std::vector<Vertex>* vertices)
{
	// Info to create a buffer (it doesnt include assigning memory)
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = sizeof(Vertex) * vertices->size();  // Size of buffer
	bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; // multiple type of buffer possible (vertex buffer)
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// Similat to swapchain images, it can share vertex buffer

	VkResult result = vkCreateBuffer(m_Device, &bufferCreateInfo, nullptr, &m_VertexBuffer);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a vertex buffer!");
	}

	// Get buffer memory requirements
	VkMemoryRequirements memRequirements = {};
	vkGetBufferMemoryRequirements(m_Device, m_VertexBuffer, &memRequirements);

	// Allocate memory to buffer
	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, // index of memory type on physical device that has required bit flags
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);		// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : CPU can interacte with memory
																							// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: Allow placement of data straight into buffer after mapping (otherwise it would have to specify manually)
	// Allocate memory to VkDeviceMemory
	result = vkAllocateMemory(m_Device, &memoryAllocateInfo, nullptr, &m_VertexBufferMemory);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate vertex buffer memory!");
	}

	// Allocate memory to given vertex buffer
	vkBindBufferMemory(m_Device, m_VertexBuffer, m_VertexBufferMemory, 0);

	// Map memory to vertex buffer
	void* data;			// 1. create pointer to a point in normal memory
	vkMapMemory(m_Device, m_VertexBufferMemory, 0, bufferCreateInfo.size, 0, &data);	// 2. "Map the vertex buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferCreateInfo.size); // 3. Copy memory from vertices vector to the point
	vkUnmapMemory(m_Device, m_VertexBufferMemory);		// 4. Unmap the vertex buffer memory
}

uint32_t Mesh::FindMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags propertyFlags)
{
	// Get properties of physical device memory
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((allowedTypes & (1 << i))  // index of memory type must match corresponding bit in allowedTypes
			&& (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) // desire property bit flags are part of memory type`s property flags
		{
			// This memory type is valid
			return i;
		}
	}

	return uint32_t();
}
