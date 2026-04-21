#pragma once
#include "pch.h"
#include "utils/utils.h"
namespace sludge
{
	// Materials really capture pipeline states more than anything. The concept of a material and a pipeline state are so
	// intertwined though, if you practically think about it, that you could really change this class name to PipelineState and itd make just as much sense.
	// However sometimes it just makes more sense to semantically to say "hey this is a material for PBR" because you understand that a material determines how something gets rendered.
	struct Material
	{
		Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState{};

		static inline Microsoft::WRL::ComPtr<ID3D12RootSignature> BindlessRootSignature{};

		static D3D12_COMPUTE_PIPELINE_STATE_DESC CreateComputePSODesc(ID3D12RootSignature* const rootSignature, ID3DBlob* const csCSO);
		static D3D12_GRAPHICS_PIPELINE_STATE_DESC CreatePSODesc(ID3DBlob* const vsCSO, ID3DBlob* const psCSO, ID3D12RootSignature* const rootSignature, DXGI_FORMAT rtvFormat = DXGI_FORMAT_R16G16B16A16_FLOAT,
			bool depthEnable = true, bool cubemap = false);
		static Material CreateMaterial(ID3D12Device* const device, std::wstring_view vsFile, std::wstring_view psFile, std::wstring_view materialName, bool cubemap = false, DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT);
		static Material CreateComputeMaterial(ID3D12Device* const device, std::wstring_view csFile, std::wstring_view materialName);
		static void CreateBindlessRootSignature(ID3D12Device* const device, std::wstring_view bindlessShader);


		void Bind(ID3D12GraphicsCommandList* const cmdList) const;
		void BindComputeShader(ID3D12GraphicsCommandList* const cmdList) const;

		static void BindRootSignature(ID3D12GraphicsCommandList* const cmdList);
		static void BindComputeRootSignature(ID3D12GraphicsCommandList* const cmdList);

		void BindPSO(ID3D12GraphicsCommandList* cmdList) const;
	};
} // sludge