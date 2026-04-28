#include "Model.h"


#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define STB_IMAGE_WRITE_IMPLEMENTATION


namespace sludge
{
	int MikkGetNumFaces(const SMikkTSpaceContext* pContext)
	{
		const Mesh& m{ *(Mesh*)(pContext->m_pUserData) };
		return m.indexCount / 3;
	}
	int MikkGetNumVerticesOfFace([[maybe_unused]] const SMikkTSpaceContext* pContext, [[maybe_unused]] const int iFace)
	{
		return 3;
	}

	void MikkGetPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
	{
		const Mesh& m{ *(Mesh*)(pContext->m_pUserData) };
		const uint32_t index{ m.Indices[iFace * 3 + iVert] };
		const DirectX::XMFLOAT3 pos{ m.Vertices[index].Pos };
		fvPosOut[0] = pos.x;
		fvPosOut[1] = pos.y;
		fvPosOut[2] = pos.z;
	}

	void MikkGetNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
	{
		const Mesh& m{ *(Mesh*)(pContext->m_pUserData) };
		const uint32_t index{ m.Indices[iFace * 3 + iVert] };
		const DirectX::XMFLOAT3 normal{ m.Vertices[index].Normal };
		fvNormOut[0] = normal.x;
		fvNormOut[1] = normal.y;
		fvNormOut[2] = normal.z;

	}
	void MikkGetTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
	{
		const Mesh& m{ *(Mesh*)(pContext->m_pUserData) };
		const uint32_t index{ m.Indices[iFace * 3 + iVert] };
		const DirectX::XMFLOAT2 texC{ m.Vertices[index].TexC };
		fvTexcOut[0] = texC.x;
		fvTexcOut[1] = texC.y;
	}
	void MikkGetTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
	{
		Mesh& m{ *(Mesh*)(pContext->m_pUserData) };
		const uint32_t index{ m.Indices[iFace * 3 + iVert] };
		m.Vertices[index].Tangent.x = fvTangent[0];
		m.Vertices[index].Tangent.y = fvTangent[1];
		m.Vertices[index].Tangent.z = fvTangent[2];
		m.Vertices[index].Tangent.w = fSign;

	}

	void calculateMikkTSpace(Mesh& m)
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

	void Model::LoadModel(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap, utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool,
		utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool, std::string_view modelPath)
	{
		const aiScene* scene = aiImportFile(modelPath.data(), aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_FlipUVs);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			ErrorMessage(L"Scene could not be loaded!");
			return;
		}
		std::string mPath = modelPath.data();
		modelDirectory_ = mPath.substr(0, mPath.find_last_of(L'/') + 1);

		ProcessNode(device, cmdList, heap, geometryPool, texPool, scene->mRootNode, scene);

		ConstantBuffer<ModelConstants> cb;
		cbHolder_ = cbPool.create(std::move(cb));
		auto constantBuffer = cbPool.get(cbHolder_);
		constantBuffer->CreateConstantBuffer(device, cmdList,
			ModelConstants{ .modelMat = DirectX::XMMatrixIdentity(), .invModelMat = DirectX::XMMatrixIdentity() },
			heap, L"Model Constants Buffer", cbHolder_.index());

	}

	void Model::UpdateData(DirectX::XMMATRIX viewProj, utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool)
	{
		// Listen this is honestly not a great way to control rotations for a model. This WILL and DOES result in gimbal lock
		// For now its just used to test whether imgui can actually edit our values correctly, nothing more
		auto scaleVector = DirectX::XMLoadFloat3(&transformData_.Scale);
		auto rotVector = DirectX::XMLoadFloat3(&transformData_.Rotation);
		auto translationVector = DirectX::XMLoadFloat3(&transformData_.Translation);

		auto cb = cbPool.get(cbHolder_);
		cb->ConstantBufferData().modelMat = DirectX::XMMatrixScalingFromVector(scaleVector) * 
			DirectX::XMMatrixRotationRollPitchYawFromVector(rotVector) * 
			DirectX::XMMatrixTranslationFromVector(translationVector);
			
		cb->ConstantBufferData().invModelMat = DirectX::XMMatrixInverse(nullptr, cb->ConstantBufferData().modelMat);
		cb->ConstantBufferData().viewProjMat = viewProj;
		cb->UpdateBuffer();
	}

	// Usually wed use a string view, but since begin needs a null terminated c string, and string views dont guarantee that it will be 
	// null terminated, we play it safe and use plain ol string.
	void Model::UpdateFromUI(std::string name, utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool)
	{
		if (ImGui::TreeNode(name.c_str()))
		{
			// Since the struct has our floats lined up in contiguous order, we send the address of the first element
			// with the understanding that the the proceeding elements will be gathered in memory appropriately.
			ImGui::SliderFloat3("Translate", &transformData_.Translation.x, -10.0f, 10.0f);
			ImGui::SliderFloat3("Rotation", &transformData_.Rotation.x, -90.0f, 90.0f);
			ImGui::SliderFloat3("Scale", &transformData_.Scale.x, 0.1f, 10.0f);
			ImGui::Checkbox("Uniform Scale", &uniformScale);
			if (uniformScale)
			{
				// Little wonky for uniform scale but gets the job done. Ill change it down the road
				transformData_.Scale.y = transformData_.Scale.x;
				transformData_.Scale.z = transformData_.Scale.x;
			}
			auto cb = cbPool.get(cbHolder_);
			ImGui::ColorEdit3("Model Diffuse Color", &cb->ConstantBufferData().albedo.x);
			
			ImGui::TreePop();
		}

	}

	void Model::Draw(ID3D12GraphicsCommandList* const cmdList)
	{
		for (auto& mesh : meshes_)
		{
			auto ibv = mesh.indexBuffer.IndexBufferView();
			cmdList->IASetIndexBuffer(&ibv);

			cmdList->DrawIndexedInstanced(mesh.indexCount, 1, 0, 0, 0);

		}
	}

	void Model::DrawNodes(ID3D12GraphicsCommandList* const cmdList, utils::ResourceIndices& modelResources)
	{

		for (auto& mesh : meshes_)
		{
			auto ibv = mesh.indexBuffer.IndexBufferView();
			cmdList->IASetIndexBuffer(&ibv);

			modelResources.VertexBufferID = mesh.vbHolder.index();
			modelResources.modelConstantID = cbHolder_.index();
			modelResources.albedoID =	 !mesh.pbrResources.albedo.empty() ? loadedTextures_[mesh.pbrResources.albedo].SRVHandle().index() : 0;
			modelResources.NormalID =	 !mesh.pbrResources.normalMap.empty() ? loadedTextures_[mesh.pbrResources.normalMap].SRVHandle().index() : 0;
			modelResources.roughnessID = !mesh.pbrResources.metallicRoughnessMap.empty() ? loadedTextures_[mesh.pbrResources.metallicRoughnessMap].SRVHandle().index() : 0;
			modelResources.EmissiveID = !mesh.pbrResources.emissiveMap.empty() ? loadedTextures_[mesh.pbrResources.emissiveMap].SRVHandle().index() : 0;
			modelResources.AoID =		 !mesh.pbrResources.aoMap.empty() ? loadedTextures_[mesh.pbrResources.aoMap].SRVHandle().index() : 0;
			cmdList->SetGraphicsRoot32BitConstants(0, 11, &modelResources, 0);

			cmdList->DrawIndexedInstanced(mesh.indexCount, 1, 0, 0,0);
			
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS Model::ConstantBufferGPUVirtualAddress(utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool)
	{
		auto cb = cbPool.get(cbHolder_);
		return cb->ConstantBufferViewDesc().BufferLocation;
	}

	void Model::ProcessNode(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap,
		utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool,
		aiNode* node, const aiScene* scene)
	{
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			auto* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes_.push_back(ProcessMesh(device, cmdList, heap, geometryPool, texPool, mesh, scene));
		}

		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(device, cmdList, heap, geometryPool, texPool, node->mChildren[i], scene);
		}


	}
	Mesh Model::ProcessMesh(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap,
		utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool,
		aiMesh* mesh, const aiScene* scene)
	{
		Mesh modelMesh;
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

		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			modelMesh.pbrResources.albedo = ProcessMaterial(device, cmdList, heap, 
				material, aiTextureType_DIFFUSE, texPool);

			modelMesh.pbrResources.normalMap = ProcessMaterial(device, cmdList, heap,
				material, aiTextureType_NORMALS, texPool);

			modelMesh.pbrResources.metallicRoughnessMap = ProcessMaterial(device, cmdList, heap,
				material, aiTextureType_GLTF_METALLIC_ROUGHNESS, texPool);

			modelMesh.pbrResources.aoMap = ProcessMaterial(device, cmdList, heap,
				material, aiTextureType_LIGHTMAP, texPool);

			modelMesh.pbrResources.emissiveMap = ProcessMaterial(device, cmdList, heap,
				material, aiTextureType_EMISSIVE, texPool);
		}

		modelMesh.Vertices = vertices;
		modelMesh.Indices = indices;
		modelMesh.vertexCount = vertices.size();
		modelMesh.indexCount = indices.size();
		if (mesh->mTangents != nullptr)
		{
			// MikkTspace is actually pretty costly to run all things considered.
			// we will probably have to find a better way to determine when we want to use it,
			// for now we just say that if this model doesn't have tangents, then we dont have a normal map
			// and therefore dont need the tangents.
			calculateMikkTSpace(modelMesh);
		}
		std::span<const uint32_t> indexSpan{ modelMesh.Indices };
		std::span<const Vertex> vertexSpan{ modelMesh.Vertices };
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
	std::string Model::ProcessMaterial(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap, aiMaterial* mat, aiTextureType type, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool)
	{
		// Either we have a texture of this type or dont
		std::string fullPath{};
		
		for (int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);
			Texture texture;
			fullPath = modelDirectory_ + str.C_Str();
			bool sRGB = type == aiTextureType_DIFFUSE;
			if (loadedTextures_.find(fullPath.data()) != loadedTextures_.end())
			{
				return fullPath;
			}
			loadedTextures_[fullPath].CreateTexture(device, cmdList, heap, fullPath, L"Test", texPool, sRGB);
			return fullPath;
		}	
		return fullPath;
	}
} // sludge