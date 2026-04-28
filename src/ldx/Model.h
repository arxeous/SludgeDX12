#pragma once
#include "pch.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ConstantBuffer.h"
#include "StructuredBuffer.h"
#include "Texture.h"
#include "ImGuiRenderer.h"
#include "utils/Holder.h"
#include "utils/Pool.h"
#include "utils/utils.h"
#include "thirdparty/MikkTSpace/mikktspace.h"
#include "UploadBuffer.h"
#include "utils/tiny_gltf.h"
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_EXTERNAL_IMAGE

namespace sludge
{
	struct PBRMaterial
	{
		std::string albedo{};
		std::string normalMap{};
		std::string aoMap{};
		std::string metallicRoughnessMap{};
		std::string emissiveMap{};
	};
	struct Mesh
	{
		IndexBuffer indexBuffer{};
		utils::Holder<utils::GeometryHandle> vbHolder{};
		// needed for mikktspace lib for tangent calculations
		std::vector<Vertex> Vertices{};
		std::vector<uint32_t> Indices{};

		uint32_t vertexCount{};
		uint32_t indexCount{};
		PBRMaterial pbrResources{};
	};
	// Rather than have our constant buffers handled by the context, the model itself will keep track of that stuff
	struct alignas(256) ModelConstants
	{
		DirectX::XMMATRIX modelMat{};
		DirectX::XMMATRIX viewProjMat{};
		DirectX::XMMATRIX invModelMat{};
		DirectX::XMFLOAT4 albedo{1.0f, 1.0f, 1.0f, 1.0f};
	};

	struct Transform
	{
		DirectX::XMFLOAT3 Rotation{};
		DirectX::XMFLOAT3 Scale{};
		DirectX::XMFLOAT3 Translation{};
	};

	class Model
	{
	public:
		void LoadModel(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap, utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool,
			utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool, std::string_view modelPath);
		Transform& GetTransformData() { return transformData_; }
		void UpdateData(DirectX::XMMATRIX viewProj, utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool);
		void UpdateFromUI(std::string name, utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool);
		void Draw(ID3D12GraphicsCommandList* const cmdList);
		void DrawNodes(ID3D12GraphicsCommandList* const cmdList, utils::ResourceIndices& modelResources);
		D3D12_GPU_VIRTUAL_ADDRESS ConstantBufferGPUVirtualAddress(utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool);
		uint32_t IndexCount() const { return indexCount_; }
		utils::Holder<utils::GeometryHandle>& VertexHolder(int idx = 0) { return meshes_[idx].vbHolder; }
		utils::Holder<utils::ModelConstantHandle>& ModelConstantHolder() { return cbHolder_; }

	private:
		void ProcessNode(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap, utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool,
			utils::Pool<utils::TextureTag, DescriptorHandle>& texPool, aiNode* node, const aiScene* scene);
		Mesh ProcessMesh(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap,
			utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool,
			aiMesh* mesh, const aiScene* scene);
		std::string ProcessMaterial(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap, aiMaterial* mat, aiTextureType type, utils::Pool<utils::TextureTag, DescriptorHandle>& texPool);
		utils::Holder<utils::ModelConstantHandle> cbHolder_{};
		std::string modelName_{};
		std::string modelDirectory_{};

		uint32_t vertexCount_{};
		uint32_t indexCount_{};
		std::vector<Mesh> meshes_{};
		Transform transformData_;
		bool uniformScale{ false };
		// This single map will keep a hold of all our textures so we arent reuploading data more than once.
		static inline  std::map<std::string_view, Texture> loadedTextures_;
	};
} // sludge