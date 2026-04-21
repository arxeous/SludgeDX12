#include "pch.h"
#include "RenderTarget.h"

namespace sludge
{
	void RenderTarget::CreateRenderTarget(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, DXGI_FORMAT format, 
		DescriptorHeap& rtvDescriptorHeap, DescriptorHeap& srvDescriptorHeap, utils::Pool<utils::TextureTag, DescriptorHandle>& texturePool,
		uint32_t width, uint32_t height, std::wstring_view name)
	{
		width_ = width;
		height = height;
		
		CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(format, static_cast<UINT64>(width),static_cast<UINT64>(height), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
		CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		D3D12_CLEAR_VALUE rtClearValue =
		{
			.Format = format,
			.Color = {0.1f, 0.1f, 0.1f, 1.0f}
		};

		ThrowIfFailed(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &rtClearValue, IID_PPV_ARGS(&resource_)));
		resource_->SetName(name.data());

		device->CreateRenderTargetView(resource_.Get(), nullptr, rtvDescriptorHeap.CPUDescriptorHandleEnd());

		// The texture we create here is pretty much empty. Its an offscreen texture so theres not image to go along with it. Realistically we only need the index at which this thing exist.
		// Since its empty!

		offScreenTextureHolder_ = texturePool.create(DescriptorHandle{});
		auto srvDescriptorLocationCPU = srvDescriptorHeap.CPUDescriptorHandleStart().Offset(offScreenTextureHolder_.index(), srvDescriptorHeap.IncrementSize());
		auto srvDescriptorLocationGPU = srvDescriptorHeap.GPUDescriptorHandleStart().Offset(offScreenTextureHolder_.index(), srvDescriptorHeap.IncrementSize());
		auto texture = texturePool.get(offScreenTextureHolder_);
		texture->CpuDescriptorHandle = srvDescriptorLocationCPU;
		texture->GpuDescriptorHandle = srvDescriptorLocationGPU;
		device->CreateShaderResourceView(resource_.Get(), nullptr, srvDescriptorLocationCPU);

		RTVCPUDescriptorHandle_ = rtvDescriptorHeap.CPUDescriptorHandleEnd();
		RTVGPUDescriptorHandle_ = rtvDescriptorHeap.GPUDescriptorHandleEnd();

		SRVCPUDescriptorHandle_ = srvDescriptorLocationCPU;
		SRVGPUDescriptorHandle_ = srvDescriptorLocationGPU;

		rtvDescriptorHeap.OffsetHandles();
		//srvDescriptorHeap.OffsetHandles();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE RenderTarget::SRVCPUDescriptorHandle(utils::Pool<utils::TextureTag, DescriptorHandle>& pool)
	{
		//return SRVCPUDescriptorHandle_;
		auto descriptorHandle = pool.get(offScreenTextureHolder_);
		return descriptorHandle->CpuDescriptorHandle;
	}
	CD3DX12_GPU_DESCRIPTOR_HANDLE RenderTarget::SRVGPUDescriptorHandle(utils::Pool<utils::TextureTag, DescriptorHandle>& pool)
	{
		//return SRVGPUDescriptorHandle_;
		auto descriptorHandle = pool.get(offScreenTextureHolder_);
		return descriptorHandle->GpuDescriptorHandle;
	}

	void RenderTarget::InitBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, DescriptorHeap& heap, utils::Pool<utils::GeometryTag, StructuredBuffer>& geometryPool)
	{
		StructuredBuffer structBuf{};
		auto handle = geometryPool.create(std::move(structBuf));
		auto structuredBuffer = geometryPool.get(handle);
		structuredBuffer->CreateStructuredBuffer<RTVertex>(device, cmdList, RT_VERTICES, L"RTV Vertex Structured Buffer");
		vbHolder_ = handle;
		auto descriptorLocation = heap.CPUDescriptorHandleStart().Offset(vbHolder_.index(), heap.IncrementSize());
		D3D12_BUFFER_SRV bufferDesc
		{
			.FirstElement = 0,
			.NumElements = RT_VERTICES.size(),
			.StructureByteStride = sizeof(RTVertex)
		};

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
		{
			.Format = DXGI_FORMAT_UNKNOWN,
			.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = bufferDesc,
		};
		device->CreateShaderResourceView(structuredBuffer->Buffer(), &srvDesc, descriptorLocation);

		RenderTarget::ib_.CreateIndexBuffer(device, cmdList, RT_INDICES, L"Render Target IB");
	}
	void RenderTarget::Bind(ID3D12GraphicsCommandList* cmdList)
	{
		auto ibv = RenderTarget::ib_.IndexBufferView();

		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->IASetIndexBuffer(&ibv);
	}
}
