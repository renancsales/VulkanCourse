#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Utils.h"

struct UniformBufferObjectModel
{
	glm::mat4 Model;
};


class Mesh
{
public:
	Mesh() = default;
	Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue,
		VkCommandPool transferCmdPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices,
		int textureID);

	~Mesh();

	int GetVertexCount();
	int GetIndexCount() { return static_cast<int>(m_IndexCount); }

	VkBuffer GetVertexBuffer();
	VkBuffer GetIndexBuffer() { return m_IndexBuffer; }

	void SetModel(glm::mat4& model) { m_UBOModel.Model = model; };
	UniformBufferObjectModel GetUniformBufferModel() { return m_UBOModel; }
	int GetTextureID() { return m_TextureID; };

	void DestroyBuffers();


private:
	void CreateVertexBuffer(VkQueue transferQueue,
		VkCommandPool transferCmdPool, std::vector<Vertex>* vertices);

	void CreateIndexBuffer(VkQueue transferQueue,
		VkCommandPool transferCmdPool, std::vector<uint32_t>* indices);

private:

	UniformBufferObjectModel m_UBOModel;

	int m_TextureID;

	size_t m_VertexCount;
	VkBuffer m_VertexBuffer = nullptr;
	VkDeviceMemory m_VertexBufferMemory;

	size_t m_IndexCount;
	VkBuffer m_IndexBuffer;
	VkDeviceMemory m_IndexBufferMemory;

	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_Device;

};

