#include "Mesh.h"


Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue,
	VkCommandPool transferCmdPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices, int textureID)
{
	m_IndexCount = indices->size();
	m_VertexCount = vertices->size();
	m_PhysicalDevice = newPhysicalDevice;
	m_Device = newDevice;
	CreateVertexBuffer(transferQueue, transferCmdPool, vertices);
	CreateIndexBuffer(transferQueue, transferCmdPool, indices);

	m_UBOModel.Model = glm::mat4(1.0f);
	m_TextureID = textureID;

}


Mesh::~Mesh()
{
	//DestroyBuffers();
}

int Mesh::GetVertexCount()
{
	return static_cast<int>(m_VertexCount);
}

VkBuffer Mesh::GetVertexBuffer()
{
	return m_VertexBuffer;
}

void Mesh::DestroyBuffers()
{
	vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
	vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);
	vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
	vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);
}

void Mesh::CreateVertexBuffer(VkQueue transferQueue,
	VkCommandPool transferCmdPool, std::vector<Vertex>* vertices)
{
	// Get size of buffer
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

	// temporary buffer to stage vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Create staging buffer and allocate memory to it 
	CreateBuffer(m_PhysicalDevice, m_Device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);


	// Map memory to vertex buffer
	void* data;			// 1. create pointer to a point in normal memory
	vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);	// 2. "Map the vertex buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferSize); // 3. Copy memory from vertices vector to the point
	vkUnmapMemory(m_Device, stagingBufferMemory);		// 4. Unmap the vertex buffer memory

	// Create buffer with TRANSFER_DST_BIT to mark recipient of transfer data (also VERTEX_BUFFER)
	// Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the gpu and only accessible by it and not CPU (host)
	CreateBuffer(m_PhysicalDevice, m_Device, bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_VertexBuffer, &m_VertexBufferMemory);

	// copy stagin buffer to vertex buffer
	CopyBuffer(m_Device, transferQueue, transferCmdPool, stagingBuffer, m_VertexBuffer, bufferSize);
	
	// cleand stagin buffer parts
	vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
	vkFreeMemory(m_Device, stagingBufferMemory, nullptr);

	//// Create buffer and allocate memory to it 
	//CreateBuffer(m_PhysicalDevice, m_Device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	//	&m_VertexBuffer, &m_VertexBufferMemory);

	//// Map memory to vertex buffer
	//void* data;			// 1. create pointer to a point in normal memory
	//vkMapMemory(m_Device, m_VertexBufferMemory, 0, bufferSize, 0, &data);	// 2. "Map the vertex buffer memory to that point
	//memcpy(data, vertices->data(), (size_t)bufferSize); // 3. Copy memory from vertices vector to the point
	//vkUnmapMemory(m_Device, m_VertexBufferMemory);		// 4. Unmap the vertex buffer memory
}

void Mesh::CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<uint32_t>* indices)
{
	// Get the buffer size
	VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

	//temporary buffer data
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Create index buffer
	// Create staging buffer and allocate memory to it 
	CreateBuffer(m_PhysicalDevice, m_Device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	// Map memory to index buffer
	void* data;			// 1. create pointer to a point in normal memory
	vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);	// 2. "Map the vertex buffer memory to that point
	memcpy(data, indices->data(), (size_t)bufferSize); // 3. Copy memory from vertices vector to the point
	vkUnmapMemory(m_Device, stagingBufferMemory);		// 4. Unmap the vertex buffer memory
	
	// Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the gpu and only accessible by it and not CPU (host)
	CreateBuffer(m_PhysicalDevice, m_Device, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_IndexBuffer, &m_IndexBufferMemory);


	// copy staging buffer to GPU access buffer
	CopyBuffer(m_Device, transferQueue, transferCmdPool, stagingBuffer, m_IndexBuffer, bufferSize);

	// cleand stagin buffer parts
	vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
	vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
}

