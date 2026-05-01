#pragma once
#include "pch.h"
#include "ldx/Scene.h"
#include "ldx/VertexBuffer.h"
#include "ldx/IndexBuffer.h"
#include "ldx/ConstantBuffer.h"
#include "ldx/StructuredBuffer.h"
#include "ldx/Texture.h"
#include "utils/Holder.h"
#include "utils/Pool.h"
#include "utils/utils.h"
#include "utils/ModelData.h"

namespace sludge::utils
{
	int addNode(Scene& scene, int parent, int level)
	{
		const int node = (int)scene.hierarchy.size();
		scene.globalTransforms.push_back(DirectX::XMMatrixIdentity());
		scene.localTransforms.push_back(DirectX::XMMatrixIdentity());
		scene.hierarchy.push_back({ .parent = parent });

		if (parent > -1)
		{
			const int s = scene.hierarchy[parent].firstChild;
			if (s == -1)
			{
				scene.hierarchy[parent].firstChild = node;
				scene.hierarchy[node].lastSibling = node;
			}
			else
			{
				int dest = scene.hierarchy[s].lastSibling;
				if (dest <= -1)
				{
					for (dest = s; scene.hierarchy[dest].nextSibling != -1; dest = scene.hierarchy[dest].nextSibling);
				}
				scene.hierarchy[dest].nextSibling = node;
				scene.hierarchy[s].lastSibling = node;
			}
		}

		scene.hierarchy[node].level = level;
		scene.hierarchy[node].nextSibling = -1;
		scene.hierarchy[node].lastSibling = -1;
		return node;
	}

	ModelMesh processMesh(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap,
		utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool,
		aiMesh* mesh)
	{
		ModelMesh modelMesh;
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		for (uint32_t j = 0; j < mesh->mNumVertices; j++)
		{
			Vertex vertex;
			const aiVector3D p = mesh->mVertices[j];
			const aiVector3D n = mesh->mNormals[j];
			const aiVector3D t = mesh->mTextureCoords[0][j];
			if (mesh->mTangents != nullptr && mesh->mBitangents != nullptr)
			{
				const aiVector3D tan = mesh->mTangents[j];
				const aiVector3D biTan = mesh->mBitangents[j];
				DirectX::XMVECTOR T = DirectX::XMVectorSet(tan.x, tan.y, tan.z, 0);
				DirectX::XMVECTOR B = DirectX::XMVectorSet(biTan.x, biTan.y, biTan.z, 0);
				DirectX::XMVECTOR N = DirectX::XMVectorSet(n.x, n.y, n.z, 0);

				float sign = DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVector3Cross(N, T), B)) < 0.0f ? -1.0f : 1.0f;
				vertex.Tangent = DirectX::XMFLOAT4(tan.x, tan.y, tan.z, sign);
			}
			else
			{
				vertex.Tangent = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
			}

			vertex.Pos = DirectX::XMFLOAT3(p.x, p.y, p.z);
			vertex.Normal = DirectX::XMFLOAT3(n.x, n.y, n.z);
			vertex.TexC = DirectX::XMFLOAT2(t.x, t.y);

			vertices.emplace_back(vertex);
		}

		for (uint32_t j = 0; j != mesh->mNumFaces; ++j)
		{
			const auto face = mesh->mFaces[j];
			for (uint32_t k = 0; k < face.mNumIndices; ++k)
			{
				indices.push_back(face.mIndices[k]);
			}
		}

		modelMesh.Vertices = vertices;
		modelMesh.Indices = indices;
		modelMesh.vertexCount = vertices.size();
		modelMesh.indexCount = indices.size();

		std::span<const uint32_t> indexSpan{ modelMesh.Indices };
		std::span<const Vertex> vertexSpan{ modelMesh.Vertices };
		// There is a good chance we will cut this part of the code out to some other function. possibly within the actual DX12 context code.
		// Since all this really does is create the GPU side resources, and the mesh shouldnt be concerned with that functionality. IT should only care about
		// loading in the mesh data and storing it.
		modelMesh.indexBuffer.CreateIndexBuffer(device, cmdList, indexSpan, L"Index Buffer");

		StructuredBuffer structBuf{};
		auto handle = geometryPool.create(std::move(structBuf));
		auto structuredBuffer = geometryPool.get(handle);
		structuredBuffer->CreateStructuredBuffer(device, cmdList, vertexSpan, L"Model Structured Buffer");
		modelMesh.vbHolder = handle;
		auto index = modelMesh.vbHolder.index();
		auto descriptorLocation = heap.CPUDescriptorHandleStart().Offset(modelMesh.vbHolder.index(), heap.IncrementSize());
		D3D12_BUFFER_SRV bufferDesc
		{
			.FirstElement = 0,
			.NumElements = modelMesh.vertexCount,
			.StructureByteStride = sizeof(Vertex)
		};

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
		{
			.Format = DXGI_FORMAT_UNKNOWN,
			.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = bufferDesc,
		};
		device->CreateShaderResourceView(structuredBuffer->Buffer(), &srvDesc, descriptorLocation);

		return modelMesh;
	}

	std::string addTexture(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap,
		utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool,
		std::string path, std::string modelDirectory, bool sRGB)
	{
		std::string fullPath = modelDirectory + path;
		if (ModelData::loadedTextures.find(fullPath.data()) != ModelData::loadedTextures.end())
		{
			return fullPath;
		}
		ModelData::loadedTextures[fullPath].CreateTexture(device, cmdList, heap, fullPath, L"Test", texPool, sRGB);
		return fullPath;
	}

	MeshMaterial processMeshMaterial(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap,
		utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool,
		const aiMaterial* mat, std::string_view modelDirectory)
	{
		MeshMaterial procMat{};
		aiString path;
		aiTextureMapping mapping;
		unsigned int uvIndex = 0;
		float blend = 1.0f;
		aiTextureOp textureOp = aiTextureOp_Add;
		aiTextureMapMode textureMapMode[2] = { aiTextureMapMode_Wrap, aiTextureMapMode_Wrap };
		unsigned int textureFlags = 0;

		if (aiGetMaterialTexture(mat, aiTextureType_DIFFUSE, 0, &path, &mapping, &uvIndex, &blend, &textureOp, textureMapMode, &textureFlags) == AI_SUCCESS)
		{
			procMat.albedo = addTexture(device, cmdList, heap, geometryPool, texPool, path.C_Str(), modelDirectory.data(), true);
			const std::string albedoMap = std::string(path.C_Str());
		}

		if (aiGetMaterialTexture(mat, aiTextureType_EMISSIVE, 0, &path, &mapping, &uvIndex, &blend, &textureOp, textureMapMode, &textureFlags) == AI_SUCCESS)
		{
			procMat.emissiveMap = addTexture(device, cmdList, heap, geometryPool, texPool, path.C_Str(), modelDirectory.data(), false);
		}

		if (aiGetMaterialTexture(mat, aiTextureType_NORMALS, 0, &path, &mapping, &uvIndex, &blend, &textureOp, textureMapMode, &textureFlags) == AI_SUCCESS)
		{
			procMat.normalMap = addTexture(device, cmdList, heap, geometryPool, texPool, path.C_Str(), modelDirectory.data(), false);
		}

		if (aiGetMaterialTexture(mat, aiTextureType_LIGHTMAP, 0, &path, &mapping, &uvIndex, &blend, &textureOp, textureMapMode, &textureFlags) == AI_SUCCESS)
		{
			procMat.aoMap = addTexture(device, cmdList, heap, geometryPool, texPool, path.C_Str(), modelDirectory.data(), false);
		}

		if (aiGetMaterialTexture(mat, aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &path, &mapping, &uvIndex, &blend, &textureOp, textureMapMode, &textureFlags) == AI_SUCCESS)
		{
			procMat.metallicRoughnessMap = addTexture(device, cmdList, heap, geometryPool, texPool, path.C_Str(), modelDirectory.data(), false);
		}

		return procMat;

	}

	void traverse(const aiScene* sourceScene, Scene& scene, aiNode* node, int parent, int atLevel)
	{
		const int newNodeID = addNode(scene, parent, atLevel);
		if (node->mName.C_Str())
		{
			uint32_t stringID = (uint32_t)scene.nodeNames.size();
			scene.nodeNames.push_back(std::string(node->mName.C_Str()));
			scene.nameForNode[newNodeID] = stringID;
		}

		for (size_t i = 0; i < node->mNumMeshes; i++)
		{
			const int newSubNodeID = addNode(scene, newNodeID, atLevel + 1);

			const uint32_t stringID = (uint32_t)scene.nodeNames.size();
			scene.nodeNames.push_back(std::string(node->mName.C_Str()) + " Mesh " + std::to_string(i));
			scene.nameForNode[newSubNodeID] = stringID;

			const int mesh = (int)node->mMeshes[i];
			scene.meshForNode[newSubNodeID] = mesh;
			scene.materialForNode[newSubNodeID] = sourceScene->mMeshes[mesh]->mMaterialIndex;

			scene.globalTransforms[newSubNodeID] = DirectX::XMMatrixIdentity();
			scene.localTransforms[newSubNodeID] = DirectX::XMMatrixIdentity();
		}

		scene.globalTransforms[newNodeID] = DirectX::XMMatrixIdentity();
		// To be changed
		scene.localTransforms[newNodeID] = DirectX::XMMatrixIdentity();

		for (unsigned int n = 0; n < node->mNumChildren; n++)
		{
			traverse(sourceScene, scene, node->mChildren[n], newNodeID, atLevel + 1);
		}
	}

	void loadMeshFile(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap,
		utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool, const char* fileName, ModelData& modelData, Scene& ourScene)
	{
		const aiScene* scene = aiImportFile(fileName, aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_FlipUVs);

		if (!scene || !scene->HasMeshes())
		{
			ErrorMessage(L"Scene could not be loaded!");
			return;
		}
		std::string mPath = fileName;
		modelData.meshes.reserve(scene->mNumMeshes);


		for (unsigned int i = 0; i != scene->mNumMeshes; i++) 
		{
			modelData.meshes.push_back(processMesh(device, cmdList, heap, geometryPool, texPool, scene->mMeshes[i]));
		}

		// extract base model path
		const std::size_t pathSeparator = std::string(fileName).find_last_of("/\\");
		const std::string modelDirectory = (pathSeparator != std::string::npos) ? std::string(fileName).substr(0, pathSeparator + 1) : std::string();


		for (unsigned int i = 0; i != scene->mNumMaterials; i++) 
		{
			const aiMaterial* m = scene->mMaterials[i];
			modelData.materials.push_back(processMeshMaterial(device, cmdList, heap, geometryPool, texPool, m, modelDirectory));
		}

		// scene hierarchy conversion
		traverse(scene, ourScene, scene->mRootNode, -1, 0);

	}
}