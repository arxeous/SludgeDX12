#pragma once
#include "pch.h"

#include "DescriptorHeap.h"
#include "Model.h"

namespace sludge
{
	class CubeMap
	{
	public:
		void CreateCubeMap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, DescriptorHeap& srvHeap, std::string_view fileName, std::wstring_view name);
		void Draw(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, Model& cube);

		ID3D12Resource* TextureResource();

		CD3DX12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHandle();
		CD3DX12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandle();

	private:
		void* textureData_;
		int width_;
		int height_;
		int componentCount_;

		Microsoft::WRL::ComPtr<ID3D12Resource> textureBuffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateBuffer_;

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle_;
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle_;

		std::array<CD3DX12_CPU_DESCRIPTOR_HANDLE, 6> cubeFaceCPUHandles_{};
		std::array<CD3DX12_GPU_DESCRIPTOR_HANDLE, 6> cubeFaceGPUHandles_{};

		std::array<DirectX::XMMATRIX, 6> lookAtMatrices_{};
		D3D12_VIEWPORT viewPort_{};
		DirectX::XMMATRIX projMatrix_{};
	};
}// sludge