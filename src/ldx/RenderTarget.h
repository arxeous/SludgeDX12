#include "pch.h"
#include "DescriptorHeap.h"
#include "StructuredBuffer.h"
#include "IndexBuffer.h"
#include "Texture.h"
#include "utils/Holder.h"
#include "utils/Pool.h"

namespace sludge
{
	struct RTIndices
	{
		uint32_t VertexBufferID;
		uint32_t TextureID;
	};

	struct RTVertex
	{
		DirectX::XMFLOAT2 position{};
		DirectX::XMFLOAT2 textureCoord{};
	};

	static constexpr std::array<RTVertex, 4> RT_VERTICES
	{
		RTVertex{.position = DirectX::XMFLOAT2(-1.0f, -1.0f),		.textureCoord = DirectX::XMFLOAT2(0.0f, 1.0f) },
		RTVertex{.position = DirectX::XMFLOAT2(-1.0f,   1.0f),		.textureCoord = DirectX::XMFLOAT2(0.0f, 0.0f) },
		RTVertex{.position = DirectX::XMFLOAT2(1.0f, 1.0f),			.textureCoord = DirectX::XMFLOAT2(1.0f, 0.0f) },
		RTVertex{.position = DirectX::XMFLOAT2(1.0f, -1.0f),		.textureCoord = DirectX::XMFLOAT2(1.0f, 1.0f) },
	};

	static constexpr std::array<uint32_t, 6> RT_INDICES
	{
		0, 2, 1, 
		0, 3, 2
	};

	// If we want to facilitate any sort of advance rendering algorithm, were gonna need to abstract the render target so that we can manage multiple of them
	// for offscreen rendering purposes.
	// Since a render target can/will be used in multiple contexts we have a view descriptor for it as an actual render target to be presented and a texture to be written/manipulated.
	// basically we have a rtv and srv for the resource.
	class RenderTarget
	{
	public:

		void CreateRenderTarget(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, DXGI_FORMAT format,
			DescriptorHeap& rtvDescriptorHeap, DescriptorHeap& srvDescriptorHeap, utils::Pool<utils::TextureTag, DescriptorHandle>& texturePool,
			uint32_t width, uint32_t height, std::wstring_view name);

		ID3D12Resource* Resource() { return resource_.Get(); };
		static ID3D12Resource* PositionBuffer() { return vb_.Buffer(); };
		[[nodiscard]] CD3DX12_CPU_DESCRIPTOR_HANDLE RTVCPUDescriptorHandle(){ return RTVCPUDescriptorHandle_; };
		[[nodiscard]] CD3DX12_GPU_DESCRIPTOR_HANDLE RTVGPUDescriptorHandle() { return RTVGPUDescriptorHandle_; };

		[[nodiscard]] CD3DX12_CPU_DESCRIPTOR_HANDLE SRVCPUDescriptorHandle(utils::Pool<utils::TextureTag, DescriptorHandle>& pool);
		[[nodiscard]] CD3DX12_GPU_DESCRIPTOR_HANDLE SRVGPUDescriptorHandle(utils::Pool<utils::TextureTag, DescriptorHandle>& pool);

		static void InitBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, DescriptorHeap& heap, utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool);
		static void Bind(ID3D12GraphicsCommandList* cmdList);
		static utils::Holder<utils::GeometryHandle>& VertexHolder() { return vbHolder_; };
		static utils::Holder<utils::TextureHandle>& TextureHolder() { return offScreenTextureHolder_; };

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> resource_{};

		CD3DX12_CPU_DESCRIPTOR_HANDLE RTVCPUDescriptorHandle_;
		CD3DX12_GPU_DESCRIPTOR_HANDLE RTVGPUDescriptorHandle_;
		CD3DX12_CPU_DESCRIPTOR_HANDLE SRVCPUDescriptorHandle_;
		CD3DX12_GPU_DESCRIPTOR_HANDLE SRVGPUDescriptorHandle_;

		uint32_t width_;
		uint32_t height;

		static inline utils::Holder<utils::TextureHandle> offScreenTextureHolder_;
		static inline utils::Holder<utils::GeometryHandle> vbHolder_;
		static inline StructuredBuffer vb_{};
		static inline IndexBuffer ib_{};

	};

} // sludge