#pragma once
#include "pch.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ConstantBuffer.h"
#include "StructuredBuffer.h"
#include "ImGuiRenderer.h"
#include "utils/Holder.h"
#include "utils/Pool.h"
#include "thirdparty/MikkTSpace/mikktspace.h"
#include "UploadBuffer.h"


namespace sludge
{
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
			utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool, std::string_view modelPath);
		void LoadModelTiny(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, DescriptorHeap& heap, utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool,
			utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool, std::string_view modelPath);
		Transform& GetTransformData() { return transformData_; }
		void UpdateData(DirectX::XMMATRIX viewProj, utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool);
		void UpdateFromUI(std::string name, utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool);
		void Draw(ID3D12GraphicsCommandList* const cmdList);
		ID3D12Resource* const VertexStructuredBuffer() const { return vertexBuffer_.Buffer(); } 
		D3D12_GPU_VIRTUAL_ADDRESS ConstantBufferGPUVirtualAddress(utils::Pool<utils::ModelConstantTag, ConstantBuffer<ModelConstants>>& cbPool);
		D3D12_INDEX_BUFFER_VIEW IndexBufferView() { return indexBuffer_.IndexBufferView(); }
		uint32_t IndexCount() const { return indexCount_; }
		utils::Holder<utils::GeometryHandle>& VertexHolder() { return vbHolder_; }
		utils::Holder<utils::ModelConstantHandle>& ModelConstantHolder() { return cbHolder_; }
		std::vector<Vertex> Vertices{};
		std::vector<uint32_t> Indices{};

	private:
		StructuredBuffer vertexBuffer_{};
		IndexBuffer indexBuffer_{};
		utils::Holder<utils::ModelConstantHandle> cbHolder_{};
		utils::Holder<utils::GeometryHandle> vbHolder_{};

		uint32_t vertexCount_{};
		uint32_t indexCount_{};

		Transform transformData_;
	};
} // sludge