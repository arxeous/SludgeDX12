#include "pch.h"
#include "CubeMap.h"
#include "stb/stb_image.h"

#include "utils/utils.h"

namespace sludge 
{
	// Really just a one for one texture creation like in the Texture class.
	// The biggest difference comes from the fact that texture cubes have some extra set up
	// but at the very least the creation of the texture itself is the same as always (upload heap and final heap!)
	void CubeMap::CreateCubeMap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, DescriptorHeap& srvHeap, std::string_view fileName, std::wstring_view name)
	{
		textureData_ = stbi_loadf(fileName.data(), &width_, &height_, nullptr, 4);
		componentCount_ = 4;

		DXGI_FORMAT textureFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;

		D3D12_RESOURCE_DESC desc
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = 0,
			.Width = static_cast<UINT> (width_),
			.Height = static_cast<UINT>(height_),
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = textureFormat,
			.SampleDesc
			{
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		};

		CD3DX12_HEAP_PROPERTIES defaultProp{ D3D12_HEAP_TYPE_DEFAULT };
		ThrowIfFailed(device->CreateCommittedResource(&defaultProp, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&textureBuffer_)));

		const uint64_t uploadBufferSize = GetRequiredIntermediateSize(textureBuffer_.Get(), 0, 1);
		CD3DX12_HEAP_PROPERTIES uploadProp{ D3D12_HEAP_TYPE_UPLOAD };
		CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
		ThrowIfFailed(device->CreateCommittedResource(&uploadProp, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&intermediateBuffer_)));

		D3D12_SUBRESOURCE_DATA textureSubData
		{
			.pData = textureData_,
			.RowPitch = static_cast<__int64>(width_ * componentCount_ ),
			.SlicePitch = static_cast<__int64>(width_ * height_ * componentCount_)
		};

		UpdateSubresources(cmdList, textureBuffer_.Get(), intermediateBuffer_.Get(), 0, 0, 1, &textureSubData);
		textureBuffer_->SetName(name.data());
		utils::TransitionResource(cmdList, textureBuffer_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);


		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
		{
			.Format = desc.Format,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D
			{
				.MipLevels = 1
			}

		};

		device->CreateShaderResourceView(textureBuffer_.Get(), &srvDesc, srvHeap.CPUDescriptorHandleEnd());
		cpuDescriptorHandle_ = srvHeap.CPUDescriptorHandleEnd();
		gpuDescriptorHandle_ = srvHeap.GPUDescriptorHandleEnd();

		srvHeap.OffsetHandles();


		//viewPort_ =
		//{
		//	.TopLeftX = 0.0f,
		//	.TopLeftY = 0.0f,
		//	.Width = static_cast<float>(width_),
		//	.Height = static_cast<float>(height_),
		//	.MinDepth = 0.0f,
		//	.MaxDepth = 0.0f
		//};

		//DirectX::XMVECTOR zero = DirectX::XMVectorZero();

		//lookAtMatrices_ =
		//{
		//	DirectX::XMMatrixLookAtLH(zero, DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),  DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)),
		//	DirectX::XMMatrixLookAtLH(zero, DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)),
		//	DirectX::XMMatrixLookAtLH(zero, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),  DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)),
		//	DirectX::XMMatrixLookAtLH(zero, DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)),
		//	DirectX::XMMatrixLookAtLH(zero, DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),  DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)),
		//	DirectX::XMMatrixLookAtLH(zero, DirectX::XMVectorSet(1.0f, 0.0f, -1.0f, 0.0f), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)),
		//};

		//projMatrix_ = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PI * 0.5f, 1.0f, 0.1f, 10.0f);

	}

	void CubeMap::Draw(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, Model& cube)
	{
		for (uint32_t i = 0; i < 6; i++)
		{
			//auto viewProj = DirectX::XMMatrixMultiply(lookAtMatrices_[i], projMatrix_);
			//cube.UpdateTransformData(viewProj);
			//cube.Draw(cmdList);
		}
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE CubeMap::CPUDescriptorHandle()
	{
		return cpuDescriptorHandle_;
	}
	CD3DX12_GPU_DESCRIPTOR_HANDLE CubeMap::GPUDescriptorHandle()
	{
		return gpuDescriptorHandle_;
	}

	ID3D12Resource* CubeMap::TextureResource()
	{
		return textureBuffer_.Get();
	}


} // sludge