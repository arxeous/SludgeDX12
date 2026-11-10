#pragma once
#include "pch.h"
#include <meshoptimizer.h>
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
	int MikkGetNumFaces(const SMikkTSpaceContext* pContext)
	{
		const ModelMesh& m{ *(ModelMesh*)(pContext->m_pUserData) };
		return m.indexCount / 3;
	}
	int MikkGetNumVerticesOfFace([[maybe_unused]] const SMikkTSpaceContext* pContext, [[maybe_unused]] const int iFace)
	{
		return 3;
	}

	void MikkGetPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
	{
		const ModelMesh& m{ *(ModelMesh*)(pContext->m_pUserData) };
		const uint32_t index{ m.Indices[iFace * 3 + iVert] };
		const DirectX::XMFLOAT3 pos{ m.Vertices[index].Pos };
		fvPosOut[0] = pos.x;
		fvPosOut[1] = pos.y;
		fvPosOut[2] = pos.z;
	}

	void MikkGetNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
	{
		const ModelMesh& m{ *(ModelMesh*)(pContext->m_pUserData) };
		const uint32_t index{ m.Indices[iFace * 3 + iVert] };
		const DirectX::XMFLOAT3 normal{ m.Vertices[index].Normal };
		fvNormOut[0] = normal.x;
		fvNormOut[1] = normal.y;
		fvNormOut[2] = normal.z;

	}
	void MikkGetTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
	{
		const ModelMesh& m{ *(ModelMesh*)(pContext->m_pUserData) };
		const uint32_t index{ m.Indices[iFace * 3 + iVert] };
		const DirectX::XMFLOAT2 texC{ m.Vertices[index].TexC };
		fvTexcOut[0] = texC.x;
		fvTexcOut[1] = texC.y;
	}
	void MikkGetTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
	{
		ModelMesh& m{ *(ModelMesh*)(pContext->m_pUserData) };
		const uint32_t index{ m.Indices[iFace * 3 + iVert] };
		m.Vertices[index].Tangent.x = fvTangent[0];
		m.Vertices[index].Tangent.y = fvTangent[1];
		m.Vertices[index].Tangent.z = fvTangent[2];
		m.Vertices[index].Tangent.w = fSign;

	}

	void calculateMikkTSpace(ModelMesh& m)
	{
		SMikkTSpaceInterface mikkInterface{};
		mikkInterface.m_getNumFaces = MikkGetNumFaces;
		mikkInterface.m_getNumVerticesOfFace = MikkGetNumVerticesOfFace;
		mikkInterface.m_getPosition = MikkGetPosition;
		mikkInterface.m_getNormal = MikkGetNormal;
		mikkInterface.m_getTexCoord = MikkGetTexCoord;
		mikkInterface.m_setTSpace = nullptr;
		mikkInterface.m_setTSpaceBasic = MikkGetTSpaceBasic;

		SMikkTSpaceContext ctx{};
		ctx.m_pInterface = &mikkInterface;
		ctx.m_pUserData = (void*)&m;

		genTangSpaceDefault(&ctx);

	}

	DirectX::XMMATRIX aiMatrixToXMMATRIX(const aiMatrix4x4& from)
	{
		// as it turns out DX12 doesnt give you direct access to XMMATRIX, so we need to use a xmfloat4x4
		// to pass into the load fcn to get back a matrix;

		DirectX::XMFLOAT4X4 to;
		to.m[0][0] = (float)from.a1;
		to.m[0][1] = (float)from.a2;
		to.m[0][2] = (float)from.a3;
		to.m[0][3] = (float)from.a4;
		to.m[1][0] = (float)from.b1;
		to.m[1][1] = (float)from.b2;
		to.m[1][2] = (float)from.b3;
		to.m[1][3] = (float)from.b4;
		to.m[2][0] = (float)from.c1;
		to.m[2][1] = (float)from.c2;
		to.m[2][2] = (float)from.c3;
		to.m[2][3] = (float)from.c4;
		to.m[3][0] = (float)from.d1;
		to.m[3][1] = (float)from.d2;
		to.m[3][2] = (float)from.d3;
		to.m[3][3] = (float)from.d4;

		return DirectX::XMLoadFloat4x4(&to);
	}

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
		utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool, std::vector<uint32_t>& globalIndices, aiMesh* mesh)
	{
		ModelMesh modelMesh;
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		std::vector<Vertex> wfVertices{};
		std::vector<uint32_t> wfIndices{};
		modelMesh.indexStart = (uint32_t)globalIndices.size();
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
			modelMesh.bb.min = DirectX::XMVectorSetX(modelMesh.bb.min, std::min(p.x, DirectX::XMVectorGetX(modelMesh.bb.min)));
			modelMesh.bb.min = DirectX::XMVectorSetY(modelMesh.bb.min, std::min(p.y, DirectX::XMVectorGetY(modelMesh.bb.min)));
			modelMesh.bb.min = DirectX::XMVectorSetZ(modelMesh.bb.min, std::min(p.z, DirectX::XMVectorGetZ(modelMesh.bb.min)));

			modelMesh.bb.max = DirectX::XMVectorSetX(modelMesh.bb.max, std::max(p.x, DirectX::XMVectorGetX(modelMesh.bb.max)));
			modelMesh.bb.max = DirectX::XMVectorSetY(modelMesh.bb.max, std::max(p.y, DirectX::XMVectorGetY(modelMesh.bb.max)));
			modelMesh.bb.max = DirectX::XMVectorSetZ(modelMesh.bb.max, std::max(p.z, DirectX::XMVectorGetZ(modelMesh.bb.max)));

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
		


		std::vector<uint32_t> remap(indices.size());
		const size_t vertexCount = meshopt_generateVertexRemap(remap.data(), indices.data(),
			indices.size(), vertices.data(), indices.size(), sizeof(Vertex));

		std::vector <uint32_t> remappedIndices(indices.size());
		std::vector<Vertex> remappedVertices(vertexCount);
		meshopt_remapIndexBuffer(remappedIndices.data(), indices.data(), indices.size(), remap.data());
		meshopt_remapVertexBuffer(remappedVertices.data(), vertices.data(), vertices.size(), sizeof(Vertex), remap.data());
		// We can do a couple of things here to optimize our mesh. First is to optimize the vertex cache
		// in the GPU by reordering the triangles (hence why only the indices are sent in) for locality.
		meshopt_optimizeVertexCache(remappedIndices.data(), remappedIndices.data(), indices.size(), vertexCount);
		// We can optimize for overdraw in a similar manner by again reordering the triangles.
		meshopt_optimizeOverdraw(remappedIndices.data(), remappedIndices.data(), indices.size(), &remappedVertices[0].Pos.x, vertexCount, sizeof(Vertex), 1.05f);
		// Finally we can optimize the actual vertex buffer for memory efficiency. As the GPU must fetch these attributes from the vertex buffer and pass it into the 
		// shader, we can speed up the fetch by again optimizeing the locality of the vertex buffer.
		meshopt_optimizeVertexFetch(remappedVertices.data(), remappedIndices.data(), indices.size(), remappedVertices.data(), vertexCount, sizeof(Vertex));

		// With all these optimizations the final thing we can do is actually create an LOD from our new index buffer that uses
		// the existing vertices with a reduced number of triangles.

		const float threshold = 0.005f;
		const size_t targetIndexCount = size_t(remappedIndices.size() * threshold);
		const float targetError = 0.01f;
		std::vector<uint32_t> indicesLOD;
		indicesLOD.resize(remappedIndices.size());
		indicesLOD.resize(meshopt_simplify(&indicesLOD[0],
			remappedIndices.data(), remappedIndices.size(),
			&remappedVertices[0].Pos.x, vertexCount, sizeof(Vertex),
			targetIndexCount, targetError));

		modelMesh.Vertices = remappedVertices;
		modelMesh.Indices = indicesLOD;
		modelMesh.vertexCount = remappedVertices.size();
		modelMesh.indexCount = indicesLOD.size();
		if (mesh->mTangents == nullptr)
		{
			calculateMikkTSpace(modelMesh);
		}

		std::span<const uint32_t> indexSpan{ modelMesh.Indices };
		std::span<const Vertex> vertexSpan{ modelMesh.Vertices };
		globalIndices.insert(globalIndices.end(), modelMesh.Indices.begin(), modelMesh.Indices.end());
		// There is a good chance we will cut this part of the code out to some other function. possibly within the actual DX12 context code.
		// Since all this really does is create the GPU side resources, and the mesh shouldnt be concerned with that functionality. IT should only care about
		// loading in the mesh data and storing it.
		modelMesh.indexBuffer.CreateIndexBuffer(device, cmdList, indexSpan, L"Index Buffer");

		StructuredBuffer structBuf{};
		auto handle = geometryPool.create(std::move(structBuf));
		auto structuredBuffer = geometryPool.get(handle);
		structuredBuffer->CreateStructuredBuffer(device, cmdList, vertexSpan, L"Model Structured Buffer");
		modelMesh.vbHolder = handle;
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

		// bb wireframe set up
		{

			Vertex v;
			v.Pos = DirectX::XMFLOAT3{ DirectX::XMVectorGetX(modelMesh.bb.min), DirectX::XMVectorGetY(modelMesh.bb.min), DirectX::XMVectorGetZ(modelMesh.bb.min) };
			wfVertices.push_back(v);
			v.Pos = DirectX::XMFLOAT3{ DirectX::XMVectorGetX(modelMesh.bb.max), DirectX::XMVectorGetY(modelMesh.bb.min), DirectX::XMVectorGetZ(modelMesh.bb.min) };
			wfVertices.push_back(v);
			v.Pos = DirectX::XMFLOAT3{ DirectX::XMVectorGetX(modelMesh.bb.max), DirectX::XMVectorGetY(modelMesh.bb.min), DirectX::XMVectorGetZ(modelMesh.bb.max) };
			wfVertices.push_back(v);
			v.Pos = DirectX::XMFLOAT3{ DirectX::XMVectorGetX(modelMesh.bb.min), DirectX::XMVectorGetY(modelMesh.bb.min), DirectX::XMVectorGetZ(modelMesh.bb.max) };
			wfVertices.push_back(v);
			v.Pos = DirectX::XMFLOAT3{ DirectX::XMVectorGetX(modelMesh.bb.min), DirectX::XMVectorGetY(modelMesh.bb.max), DirectX::XMVectorGetZ(modelMesh.bb.min) };
			wfVertices.push_back(v);
			v.Pos = DirectX::XMFLOAT3{ DirectX::XMVectorGetX(modelMesh.bb.max), DirectX::XMVectorGetY(modelMesh.bb.max), DirectX::XMVectorGetZ(modelMesh.bb.min) };
			wfVertices.push_back(v);
			v.Pos = DirectX::XMFLOAT3{ DirectX::XMVectorGetX(modelMesh.bb.max), DirectX::XMVectorGetY(modelMesh.bb.max), DirectX::XMVectorGetZ(modelMesh.bb.max) };
			wfVertices.push_back(v);
			v.Pos = DirectX::XMFLOAT3{ DirectX::XMVectorGetX(modelMesh.bb.min), DirectX::XMVectorGetY(modelMesh.bb.max), DirectX::XMVectorGetZ(modelMesh.bb.max) };
			wfVertices.push_back(v);

			std::vector<uint32_t> i =
			{     
				0, 1,
				1, 2,
				2, 3,
				3, 0,
				4, 5,
				5, 6,
				6, 7,
				7, 4,
				0, 4,
				1, 5,
				2, 6,
				3, 7
			};

			wfIndices = i;

			std::span<const uint32_t> indexSpan{ wfIndices };
			std::span<const Vertex> vertexSpan{ wfVertices };

			modelMesh.wireFrameIStart = (uint32_t)globalIndices.size();
			globalIndices.insert(globalIndices.end(), wfIndices.begin(), wfIndices.end());
			modelMesh.wireFrameIBuffer.CreateIndexBuffer(device, cmdList, indexSpan, L"WF Index Buffer"); // Technically this isnt needed. The globalIB is what is used as the ibv for our renders. But its good to have this for now JIC.

			StructuredBuffer structBuf{};
			auto handle = geometryPool.create(std::move(structBuf));
			auto structuredBuffer = geometryPool.get(handle);
			structuredBuffer->CreateStructuredBuffer(device, cmdList, vertexSpan, L"WF Structured Buffer");
			modelMesh.wfHolder = handle;
			auto descriptorLocation = heap.CPUDescriptorHandleStart().Offset(modelMesh.wfHolder.index(), heap.IncrementSize());
			D3D12_BUFFER_SRV bufferDesc
			{
				.FirstElement = 0,
				.NumElements = 8,
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
		}


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
		scene.localTransforms[newNodeID] = aiMatrixToXMMATRIX(node->mTransformation);
		for (unsigned int n = 0; n < node->mNumChildren; n++)
		{
			traverse(sourceScene, scene, node->mChildren[n], newNodeID, atLevel + 1);
		}
	}

	void loadMeshFile(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap,
		utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool, utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool,
		const char* fileName, ModelData& modelData, Scene& ourScene)
	{
		const aiScene* scene = aiImportFile(fileName, aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_FlipUVs);

		if (!scene || !scene->HasMeshes())
		{
			ErrorMessage(L"Scene could not be loaded!");
			return;
		}
		std::string mPath = fileName;
		modelData.meshes.reserve(scene->mNumMeshes);
		std::vector<uint32_t> globalIndices{};

		for (unsigned int i = 0; i != scene->mNumMeshes; i++) 
		{
			modelData.meshes.push_back(processMesh(device, cmdList, heap, geometryPool, globalIndices, scene->mMeshes[i]));
		}
		std::span<uint32_t> indexSpan{ globalIndices };
		modelData.globalIB.CreateIndexBuffer(device, cmdList, indexSpan, L"Global Index Buffer");
		// extract base model path
		const std::size_t pathSeparator = std::string(fileName).find_last_of("/\\");
		const std::string modelDirectory = (pathSeparator != std::string::npos) ? std::string(fileName).substr(0, pathSeparator + 1) : std::string();


		for (unsigned int i = 0; i != scene->mNumMaterials; i++) 
		{
			const aiMaterial* m = scene->mMaterials[i];
			ourScene.materialNames.push_back(m->GetName().C_Str());
			modelData.materials.push_back(processMeshMaterial(device, cmdList, heap, geometryPool, texPool, m, modelDirectory));
		}


		ConstantBuffer<ModelConstants> cb;
		modelData.cbHolder = cbPool.create(std::move(cb));
		auto constantBuffer = cbPool.get(modelData.cbHolder);
		constantBuffer->CreateConstantBuffer(device, cmdList,
			ModelConstants{ .modelMat = DirectX::XMMatrixIdentity(), .invModelMat = DirectX::XMMatrixIdentity() },
			heap, L"Model Constants Buffer", modelData.cbHolder.index());

		traverse(scene, ourScene, scene->mRootNode, -1, 0);

	}

	void updateModelData(ModelData& modelData, DirectX::XMMATRIX viewProj, utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool)
	{
		auto scaleVector = DirectX::XMLoadFloat3(&modelData.transform.Scale);
		auto rotVector = DirectX::XMLoadFloat3(&modelData.transform.Rotation);
		auto translationVector = DirectX::XMLoadFloat3(&modelData.transform.Translation);

		auto cb = cbPool.get(modelData.cbHolder);
		cb->ConstantBufferData().modelMat = DirectX::XMMatrixScalingFromVector(scaleVector) *
			DirectX::XMMatrixRotationRollPitchYawFromVector(rotVector) *
			DirectX::XMMatrixTranslationFromVector(translationVector);

		cb->ConstantBufferData().invModelMat = DirectX::XMMatrixInverse(nullptr, cb->ConstantBufferData().modelMat);
		cb->ConstantBufferData().viewProjMat = viewProj;
		cb->UpdateBuffer();
	}

	void getFrustrumPlanes(DirectX::XMMATRIX vp, std::vector<DirectX::XMVECTOR>& planes)
	{
		planes.push_back(DirectX::XMVectorAdd(vp.r[3], vp.r[0]));		// left plane
		planes.push_back(DirectX::XMVectorSubtract(vp.r[3], vp.r[0]));	// right plane
		planes.push_back(DirectX::XMVectorAdd(vp.r[3], vp.r[1]));		// bottom plane
		planes.push_back(DirectX::XMVectorSubtract(vp.r[3], vp.r[1]));  // top plane
		planes.push_back(vp.r[2]);										// near plane
		planes.push_back(DirectX::XMVectorSubtract(vp.r[3], vp.r[2]));  // far plane

		planes[0] = DirectX::XMPlaneNormalize(planes[0]);
		planes[1] = DirectX::XMPlaneNormalize(planes[1]);
		planes[2] = DirectX::XMPlaneNormalize(planes[2]);
		planes[3] = DirectX::XMPlaneNormalize(planes[3]);
		planes[4] = DirectX::XMPlaneNormalize(planes[4]);
		planes[5] = DirectX::XMPlaneNormalize(planes[5]);

	}

	void getFrustrumCorners(DirectX::XMMATRIX vp, std::vector<DirectX::XMVECTOR>& points)
	{
		const std::vector<DirectX::XMVECTOR> corners =
		{
			DirectX::XMVECTOR{-1, 1, 0, 1}, DirectX::XMVECTOR{1, 1, 0, 1},	// top left top right near plane
			DirectX::XMVECTOR{-1,-1, 0, 1}, DirectX::XMVECTOR{1,-1, 0, 1}, // bottom left bottom right near plane
			DirectX::XMVECTOR{-1, 1, 1, 1}, DirectX::XMVECTOR{1, 1, 1, 1},  // top left top right far plane
			DirectX::XMVECTOR{-1,-1, 1, 1}, DirectX::XMVECTOR{1,-1, 1, 1}   // bottom left bottom right far plane
		};
		DirectX::XMMATRIX inverseVP = DirectX::XMMatrixInverse(nullptr, vp);

		for (int i = 0; i != 8; i++)
		{
			DirectX::XMVECTOR q = DirectX::XMVector4Transform(corners[i], inverseVP);
			q = DirectX::XMVectorSetW(q, DirectX::XMVectorGetW(q));
			q = DirectX::XMVectorScale(q, 1/DirectX::XMVectorGetW(q));
			points.push_back(q);
		}
	}

	bool isBoxInFrustrum(std::vector<DirectX::XMVECTOR>& planes, std::vector<DirectX::XMVECTOR>& corners, const BoundingBox bb)
	{
		using DirectX::XMVector4Dot;
		using DirectX::XMVectorGetX;
		using DirectX::XMVectorGetY;
		using DirectX::XMVectorGetZ;

		// check if bb is completely outside of the frustrum.
		for (int i = 0; i < 6; i++)
		{
			int r = 0;
			r += (XMVectorGetX(XMVector4Dot(planes[i], DirectX::XMVECTOR{DirectX::XMVectorGetX(bb.max), DirectX::XMVectorGetY(bb.min), DirectX::XMVectorGetZ(bb.min), 0.0f})) < 0) ? 1 : 0;
			r += (XMVectorGetX(XMVector4Dot(planes[i], DirectX::XMVECTOR{DirectX::XMVectorGetX(bb.max), DirectX::XMVectorGetY(bb.max), DirectX::XMVectorGetZ(bb.min), 0.0f})) < 0) ? 1 : 0;
			r += (XMVectorGetX(XMVector4Dot(planes[i], DirectX::XMVECTOR{DirectX::XMVectorGetX(bb.max), DirectX::XMVectorGetY(bb.min), DirectX::XMVectorGetZ(bb.max), 0.0f})) < 0) ? 1 : 0;
			r += (XMVectorGetX(XMVector4Dot(planes[i], DirectX::XMVECTOR{DirectX::XMVectorGetX(bb.max), DirectX::XMVectorGetY(bb.max), DirectX::XMVectorGetZ(bb.max), 0.0f})) < 0) ? 1 : 0;
			r += (XMVectorGetX(XMVector4Dot(planes[i], DirectX::XMVECTOR{DirectX::XMVectorGetX(bb.min), DirectX::XMVectorGetY(bb.min), DirectX::XMVectorGetZ(bb.min), 0.0f})) < 0) ? 1 : 0;
			r += (XMVectorGetX(XMVector4Dot(planes[i], DirectX::XMVECTOR{DirectX::XMVectorGetX(bb.min), DirectX::XMVectorGetY(bb.max), DirectX::XMVectorGetZ(bb.max), 0.0f})) < 0) ? 1 : 0;
			r += (XMVectorGetX(XMVector4Dot(planes[i], DirectX::XMVECTOR{DirectX::XMVectorGetX(bb.min), DirectX::XMVectorGetY(bb.max), DirectX::XMVectorGetZ(bb.min), 0.0f})) < 0) ? 1 : 0;
			r += (XMVectorGetX(XMVector4Dot(planes[i], DirectX::XMVECTOR{DirectX::XMVectorGetX(bb.min), DirectX::XMVectorGetY(bb.min), DirectX::XMVectorGetZ(bb.max), 0.0f})) < 0) ? 1 : 0;

			if (r == 8) return false;
		}

		// check if frustrum is completely inside of the bb;
		int r = 0;
		r = 0;
		for (int i = 0; i < 8; i++)
			r += ((XMVectorGetX(corners[i]) > XMVectorGetX(bb.max)) ? 1 : 0);
		if (r == 8) return false;
		r = 0;
		for (int i = 0; i < 8; i++)
			r += ((XMVectorGetX(corners[i]) < XMVectorGetX(bb.min)) ? 1 : 0);
		if (r == 8) return false;
		r = 0;
		for (int i = 0; i < 8; i++)
			r += ((XMVectorGetY(corners[i]) > XMVectorGetY(bb.max)) ? 1 : 0);
		if (r == 8) return false;
		r = 0;
		for (int i = 0; i < 8; i++)
			r += ((XMVectorGetY(corners[i]) < XMVectorGetY(bb.min)) ? 1 : 0);
		if (r == 8) return false;
		r = 0;
		for (int i = 0; i < 8; i++)
			r += ((XMVectorGetZ(corners[i]) > XMVectorGetZ(bb.max)) ? 1 : 0);
		if (r == 8) return false;
		r = 0;
		for (int i = 0; i < 8; i++)
			r += ((XMVectorGetZ(corners[i]) < XMVectorGetZ(bb.min)) ? 1 : 0);
		if (r == 8) return false;

		return true;
	}
}