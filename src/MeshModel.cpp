#include "MeshModel.h"


MeshModel::MeshModel(std::vector<Mesh>& meshList)
	: m_MeshList(meshList)
{
	m_Model = glm::mat4(1.0f);
}

const Mesh& MeshModel::GetMesh(size_t index) const
{
	// TODO: insert return statement here
	if (index >= m_MeshList.size())
	{
		throw std::runtime_error("Attempted to access invalid mesh index");
	}

	return m_MeshList[index];
}

void MeshModel::SetModel(glm::mat4& newModel)
{
	m_Model = newModel;
}

void MeshModel::DestroyMeshModel()
{
	for (auto& mesh : m_MeshList)
	{
		mesh.DestroyBuffers();
	}
}

std::vector<std::string> MeshModel::LoadMaterials(const aiScene* scene)
{
	// Create 1:1 sized list of textures
	std::vector<std::string> textureList(scene->mNumMaterials);
	
	for (size_t i = 0; i < scene->mNumMaterials; i++)
	{
		aiMaterial* material = scene->mMaterials[i];

		// Initialize the texture to empty string (will be replaced if texture exists)
		textureList[i] = "";
		// Check for a diffuse texture (standard detail texture)
		if (material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			// Get the path of the texture file
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
			{
				// Cut off any directory information already present
				//int index = std::string(path.data).rfind("\\");
				//std::string fileName = std::string(path.data).substr(index + 1);

				textureList[i] = std::string(path.data);
				//textureList[i] = fileName;
			}
		}

	}

	return textureList;
}

std::vector<Mesh> MeshModel::LoadNode(VkPhysicalDevice newPhysicaldDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiNode* node, const aiScene* scene, std::vector<int>& materialToTexture)
{
	std::vector<Mesh> meshList;

	// Go through each mesh at this node and create it, then add it out meshList
	for (size_t i = 0; i < node->mNumMeshes; i++)
	{
		// Load mesh and push back to mesh list
		auto mesh = LoadMesh(newPhysicaldDevice, newDevice, transferQueue,
			transferCommandPool, scene->mMeshes[node->mMeshes[i]], scene, materialToTexture);
		meshList.push_back(mesh);
	}

	// Go through each node attached to this node and load it, then append their meshs to his node`s  mesh list
	for (size_t i = 0; i < node->mNumChildren; i++)
	{
		std::vector<Mesh> newList = LoadNode(newPhysicaldDevice, newDevice, transferQueue,
			transferCommandPool, node->mChildren[i], scene, materialToTexture);
		meshList.insert(meshList.end(), newList.begin(), newList.end());
	}

	return meshList;
}

Mesh MeshModel::LoadMesh(VkPhysicalDevice newPhysicaldDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiMesh* mesh, const aiScene* scene, std::vector<int>& materialToTexture)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	// Resize vertex lit to hold all vertices for mesh
	vertices.resize(mesh->mNumVertices);

	// Go through each vertex and copy it across to our vertices
	for (size_t i = 0; i < mesh->mNumVertices; i++)
	{
		// Set position
		vertices[i].Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

		// Set tex coord (if they exist)
		if (mesh->mTextureCoords[0])
		{
			vertices[i].TextureCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}
		else
		{
			vertices[i].TextureCoords = {0.0f, 0.0f};
		}

		// Set color
		vertices[i].Color = { 1.0f, 1.0f, 1.0f };


		if(mesh->HasNormals())
		{
			
		}
	}

	// Iterate over indices through faces and copy across
	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{
		// Get a face
		aiFace face = mesh->mFaces[i];
		// Go through face`s indices and add to list
		for (size_t j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// Create mesh and return it
	Mesh newMesh = Mesh(newPhysicaldDevice, newDevice, transferQueue,
		transferCommandPool, &vertices, &indices, materialToTexture[mesh->mMaterialIndex]);

	return newMesh;
}

MeshModel::~MeshModel()
{
	//DestroyMeshModel();
}
