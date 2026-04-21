#include "pch.h"
#include "DepthStencilBuffer.h"

namespace sludge
{
	void DepthStencilBuffer::CreateDepthStencilBuffer(ID3D12Device* const device, DescriptorHeap dsvHeap, uint32_t width, uint32_t height, std::wstring_view name)
	{
		D3D12_CLEAR_VALUE clearValue
		{
			.Format = DXGI_FORMAT_D32_FLOAT,
			.DepthStencil
			{
				.Depth = 1.0f,
				.Stencil = 0
			}
		};

		CD3DX12_HEAP_PROPERTIES prop{ D3D12_HEAP_TYPE_DEFAULT };
		CD3DX12_RESOURCE_DESC depthTex = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);


		ThrowIfFailed(device->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &depthTex, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&depthStencilBuffer_)));
		depthStencilBuffer_->SetName(name.data());

		D3D12_DEPTH_STENCIL_VIEW_DESC desc
		{
			.Format = DXGI_FORMAT_D32_FLOAT,
			.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
			.Flags = D3D12_DSV_FLAG_NONE,
			.Texture2D
			{
				.MipSlice = 0
			}
		};

		device->CreateDepthStencilView(depthStencilBuffer_.Get(), &desc, dsvHeap.CPUDescriptorHandleEnd());
		bufferHandle_ = dsvHeap.CPUDescriptorHandleEnd();
		dsvHeap.OffsetHandles();
	}
}