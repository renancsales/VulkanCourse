#pragma once

#include <glm/glm.hpp>
#include <assimp/scene.h>

#include <vector>

#include "Mesh.h"

class MeshModel
{
public:
	MeshModel() = default;
	MeshModel(std::vector<Mesh>& meshList);
	

	size_t GetMeshCount() { return m_MeshList.size(); }
	const Mesh& GetMesh(size_t index) const;

	glm::mat4 GetModel() { return m_Model; }
	void SetModel(glm::mat4& newModel);

	void DestroyMeshModel();

	static std::vector<std::string> LoadMaterials(const aiScene* scene);
	static std::vector<Mesh> LoadNode(VkPhysicalDevice newPhysicaldDevice, VkDevice newDevice, VkQueue transferQueue,
		VkCommandPool transferCommandPool, aiNode* node, const aiScene* scene, std::vector<int>& materialToTexture);
	static Mesh LoadMesh(VkPhysicalDevice newPhysicaldDevice, VkDevice newDevice, VkQueue transferQueue,
		VkCommandPool transferCommandPool, aiMesh* mesh, const aiScene* scene, std::vector<int>& materialToTexture);

	~MeshModel();

private:
	std::vector<Mesh> m_MeshList;
	glm::mat4 m_Model;
};

