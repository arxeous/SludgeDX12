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
	void Model::LoadModelTiny(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap, utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool,
		utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool, std::string_view modelPath)
	{
		//std::string warning{};
		//std::string error{};
		//std::string mPath = modelPath.data();
		//modelDirectory_ = mPath.substr(0, mPath.find_last_of(L'/') + 1);

		//tinygltf::TinyGLTF context{};

		//tinygltf::Model model{};

		//if (!context.LoadASCIIFromFile(&model, &error, &warning, modelPath.data()))
		//{
		//	ErrorMessage(L"Scene could not be loaded!");
		//}

		//// Build meshes.

		//tinygltf::Scene& scene = model.scenes[model.defaultScene];

		//for (const int& nodeIndex : scene.nodes)
		//{
		//	LoadNode(device, cmdList, heap, geometryPool, texPool, nodeIndex, model);
		//}

		//ConstantBuffer<ModelConstants> cb;
		//cbHolder_ = cbPool.create(std::move(cb));
		//auto constantBuffer = cbPool.get(cbHolder_);
		//constantBuffer->CreateConstantBuffer(device, cmdList,
		//	ModelConstants{ .modelMat = DirectX::XMMatrixIdentity(), .invModelMat = DirectX::XMMatrixIdentity() },
		//	heap, L"Model Constants Buffer", cbHolder_.index());

	}

	void Model::LoadModel(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap, utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool,
		utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool, std::string_view modelPath)
	{
		// This will eventually be fleshed out to load full gltf scenes properly through node traversal but for now we just need some basic 
		// mesh loading.
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
		ImGui::Begin(name.c_str());

		// Since the struct has our floats lined up in contiguous order, we send the address of the first element
		// with the understanding that the the proceeding elements will be gathered in memory appropriately.
		ImGui::SliderFloat3("Translate", &transformData_.Translation.x, -10.0f, 10.0f);
		ImGui::SliderFloat3("Rotation", &transformData_.Rotation.x, -90.0f, 90.0f);
		auto cb = cbPool.get(cbHolder_);
		ImGui::ColorEdit3("Model Diffuse Color", &cb->ConstantBufferData().albedo.x);

		ImGui::End();
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
			modelResources.albedoID = mesh.pbrResources.albedo.SRVHandle().index();
			modelResources.NormalID = mesh.pbrResources.normalMap.SRVHandle().index();
			modelResources.roughnessID = mesh.pbrResources.metallicRoughnessMap.SRVHandle().index();
			modelResources.AoID = mesh.pbrResources.aoMap.SRVHandle().index();
			cmdList->SetGraphicsRoot32BitConstants(0, 11, &modelResources, 0);

			cmdList->DrawIndexedInstanced(mesh.indexCount, 1, 0, 0,0);
			
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS Model::ConstantBufferGPUVirtualAddress(utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool)
	{
		auto cb = cbPool.get(cbHolder_);
		return cb->ConstantBufferViewDesc().BufferLocation;
	}

	void Model::LoadNode(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap, utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool, 
		utils::Pool<utils::TextureTag, DescriptorHandle>& texPool, uint32_t idx, tinygltf::Model& model)
	{
		tinygltf::Node& node = model.nodes[idx];
		if (node.mesh < 0)
		{
			node.mesh = 0;
		}

		tinygltf::Mesh& nodeMesh = model.meshes[node.mesh];
		for (size_t i = 0; i < nodeMesh.primitives.size(); i++)
		{
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			std::string albedoPath{};
			std::string normalMapPath{};
			std::string metallicRoughnessMapPath{};
			std::string aoMapPath{};

			// Get Accesor, buffer view and buffer for each attribute (position, textureCoord, normal).
			tinygltf::Primitive primitive = nodeMesh.primitives[i];
			tinygltf::Accessor& indexAccesor = model.accessors[primitive.indices];

			// Position data.
			tinygltf::Accessor& positionAccesor = model.accessors[primitive.attributes["POSITION"]];
			tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccesor.bufferView];
			tinygltf::Buffer& positionBuffer = model.buffers[positionBufferView.buffer];

			int positionByteStride = positionAccesor.ByteStride(positionBufferView);
			uint8_t* positions = &positionBuffer.data[positionBufferView.byteOffset + positionAccesor.byteOffset];

			// TextureCoord data.
			tinygltf::Accessor& textureCoordAccesor = model.accessors[primitive.attributes["TEXCOORD_0"]];
			tinygltf::BufferView& textureCoordBufferView = model.bufferViews[textureCoordAccesor.bufferView];
			tinygltf::Buffer& textureCoordBuffer = model.buffers[textureCoordBufferView.buffer];
			int textureCoordBufferStride = textureCoordAccesor.ByteStride(textureCoordBufferView);
			uint8_t* texcoords = &textureCoordBuffer.data[textureCoordBufferView.byteOffset + textureCoordAccesor.byteOffset];

			// Normal data.
			tinygltf::Accessor& normalAccesor = model.accessors[primitive.attributes["NORMAL"]];
			tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccesor.bufferView];
			tinygltf::Buffer& normalBuffer = model.buffers[normalBufferView.buffer];
			int normalByteStride = normalAccesor.ByteStride(normalBufferView);
			uint8_t* normals = &normalBuffer.data[normalBufferView.byteOffset + normalAccesor.byteOffset];

			// Tangent data.
			tinygltf::Accessor& tangentAccesor = model.accessors[primitive.attributes["TANGENT"]];
			tinygltf::BufferView& tangentBufferView = model.bufferViews[tangentAccesor.bufferView];
			tinygltf::Buffer& tangentBuffer = model.buffers[tangentBufferView.buffer];
			int tangentByteStride = tangentAccesor.ByteStride(tangentBufferView);
			uint8_t  const* tangents = &tangentBuffer.data[tangentBufferView.byteOffset + tangentAccesor.byteOffset];

			// Fill in the vertices array.
			for (size_t i = 0; i < positionAccesor.count; ++i)
			{
				DirectX::XMFLOAT3 position{};
				position.x = (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[0];
				position.y = (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[1];
				position.z = -(reinterpret_cast<float const*>(positions + (i * positionByteStride)))[2];

				DirectX::XMFLOAT2 textureCoord{};
				textureCoord.x = (reinterpret_cast<float const*>(texcoords + (i * textureCoordBufferStride)))[0];
				textureCoord.y = (reinterpret_cast<float const*>(texcoords + (i * textureCoordBufferStride)))[1];
				//textureCoord.y = 1.0f - textureCoord.y;

				DirectX::XMFLOAT3 normal{};
				normal.x = (reinterpret_cast<float const*>(normals + (i * normalByteStride)))[0];
				normal.y = (reinterpret_cast<float const*>(normals + (i * normalByteStride)))[1];
				normal.z = -(reinterpret_cast<float const*>(normals + (i * normalByteStride)))[2];

				DirectX::XMFLOAT4 tangent{};
				tangent.x = (reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[0];
				tangent.y = (reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[1];
				tangent.z = -(reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[2];
				tangent.w = (reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[3];

				DirectX::XMFLOAT3 tangentCoord{ tangent.x, tangent.y, tangent.z };

				Vertex vertex;
				vertex.Pos = position;
				vertex.Normal = normal;
				vertex.TexC = textureCoord;
				vertex.Tangent = tangent;
				vertices.push_back(vertex);
			}

			// Get the index buffer data.
			tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccesor.bufferView];
			tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
			int indexByteStride = indexAccesor.ByteStride(indexBufferView);
			uint8_t* indexes = indexBuffer.data.data() + indexBufferView.byteOffset + indexAccesor.byteOffset;

			// Fill indices array.
			for (size_t i = 0; i < indexAccesor.count; ++i)
			{
				if (indexAccesor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					indices.push_back(static_cast<uint32_t>((reinterpret_cast<uint16_t const*>(indexes + (i * indexByteStride)))[0]));
				}
				else if (indexAccesor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					indices.push_back(static_cast<uint32_t>((reinterpret_cast<uint32_t const*>(indexes + (i * indexByteStride)))[0]));
				}
			}

			tinygltf::Material gltfPBRMaterial = model.materials[primitive.material];

			// For whatever reason, tiny gltf stores both the albedo and metallic roughness texture in the same pbr object, isntead of just having
			// the albedo inside the material like the rest of the textures. ???
			if (gltfPBRMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0)
			{
				tinygltf::Texture& albedoTexture = model.textures[gltfPBRMaterial.pbrMetallicRoughness.baseColorTexture.index];
				tinygltf::Image& albedoImage = model.images[albedoTexture.source];

				albedoPath = modelDirectory_ + albedoImage.uri;
			}

			if (gltfPBRMaterial.normalTexture.index >= 0)
			{
				tinygltf::Texture& normalTexture = model.textures[gltfPBRMaterial.normalTexture.index];
				tinygltf::Image& normalImage = model.images[normalTexture.source];

				normalMapPath = modelDirectory_ + normalImage.uri;
			}

			if (gltfPBRMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
			{
				tinygltf::Texture& metalRoughnessTexture = model.textures[gltfPBRMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index];
				tinygltf::Image& metalRoughnessImage = model.images[metalRoughnessTexture.source];

				metallicRoughnessMapPath = modelDirectory_ + metalRoughnessImage.uri;
			}

			if (gltfPBRMaterial.occlusionTexture.index >= 0)
			{
				tinygltf::Texture& aoTexture = model.textures[gltfPBRMaterial.occlusionTexture.index];
				tinygltf::Image& aoImage = model.images[aoTexture.source];

				aoMapPath = modelDirectory_ + aoImage.uri;
			}

			Mesh mesh{};
			//std::string_view modelNum = modelName_ + "Mesh " + std::to_string(i);
			mesh.vertexCount = vertices.size();
			mesh.indexCount = indices.size();
			calculateMikkTSpace(mesh);

			std::span<const uint32_t> indexSpan{ indices };
			std::span<const Vertex> vertexSpan{ vertices };

			mesh.indexBuffer.CreateIndexBuffer(device, cmdList, indexSpan, L"Index Buffer");

			StructuredBuffer structBuf{};
			auto handle = geometryPool.create(std::move(structBuf));
			auto structuredBuffer = geometryPool.get(handle);
			structuredBuffer->CreateStructuredBuffer(device, cmdList, vertexSpan, L"Model Structured Buffer");

			mesh.vbHolder = handle;
			auto descriptorLocation = heap.CPUDescriptorHandleStart().Offset(mesh.vbHolder.index(), heap.IncrementSize());
			D3D12_BUFFER_SRV bufferDesc
			{
				.FirstElement = 0,
				.NumElements = vertexCount_,
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

			if (!albedoPath.empty())
			{
				mesh.pbrResources.albedo.CreateTexture(device, cmdList, heap, albedoPath, L"Albedo Texture", texPool);
			}

			if (!normalMapPath.empty())
			{
				mesh.pbrResources.normalMap.CreateTexture(device, cmdList, heap, albedoPath, L"Normal Texture", texPool);
			}

			if (!metallicRoughnessMapPath.empty())
			{
				mesh.pbrResources.metallicRoughnessMap.CreateTexture(device, cmdList, heap, albedoPath, L"Metallic Roughness Texture", texPool);
			}
				
			if (!aoMapPath.empty())
			{
				mesh.pbrResources.aoMap.CreateTexture(device, cmdList, heap, albedoPath, L"AO Texture", texPool);
			}

			meshes_.emplace_back(std::move(mesh));
		}

		for (const int& childrenNodeIndex : node.children)
		{
			LoadNode(device, cmdList, heap, geometryPool, texPool, childrenNodeIndex, model);
		}

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
		}

		modelMesh.Vertices = vertices;
		modelMesh.Indices = indices;
		modelMesh.vertexCount = vertices.size();
		modelMesh.indexCount = indices.size();
		calculateMikkTSpace(modelMesh);
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
	Texture Model::ProcessMaterial(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap, aiMaterial* mat, aiTextureType type, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool)
	{
		// Either we have a texture of this type or dont
		for (int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);
			Texture texture;
			std::string fullPath = modelDirectory_ + str.C_Str();
			texture.CreateTexture(device, cmdList, heap, fullPath, L"Test", texPool);
			return texture;
		}
		return Texture();
	}
} // sludge