#include "Material.h"

namespace sludge
{
	void Material::CreateBindlessRootSignature(ID3D12Device* const device, std::wstring_view BindlessSignature)
	{
		Microsoft::WRL::ComPtr<ID3DBlob> vs;
		ThrowIfFailed(::D3DReadFileToBlob(BindlessSignature.data(), &vs));

		ThrowIfFailed(device->CreateRootSignature(0, vs.Get()->GetBufferPointer(), vs.Get()->GetBufferSize(), IID_PPV_ARGS(&BindlessRootSignature)));
		auto name = std::wstring(L"Bindless Root Signature");
		BindlessRootSignature->SetName(name.c_str());
	}

	void Material::Bind(ID3D12GraphicsCommandList* const cmdList) const
	{
		cmdList->SetGraphicsRootSignature(BindlessRootSignature.Get());
		cmdList->SetPipelineState(PipelineState.Get());
	}

	void Material::BindComputeShader(ID3D12GraphicsCommandList* const cmdList) const
	{
		cmdList->SetComputeRootSignature(BindlessRootSignature.Get());
		cmdList->SetPipelineState(PipelineState.Get());
	}

	void Material::BindRootSignature(ID3D12GraphicsCommandList* const cmdList)
	{
		cmdList->SetGraphicsRootSignature(BindlessRootSignature.Get());
	}

	void Material::BindComputeRootSignature(ID3D12GraphicsCommandList* const cmdList)
	{
		cmdList->SetComputeRootSignature(BindlessRootSignature.Get());

	}
	void Material::BindPSO(ID3D12GraphicsCommandList* const cmdList) const
	{
		cmdList->SetPipelineState(PipelineState.Get());
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC Material::CreateComputePSODesc(ID3D12RootSignature* const rootSignature, ID3DBlob* const csCSO)
	{
		return D3D12_COMPUTE_PIPELINE_STATE_DESC
		{
			.pRootSignature = rootSignature,
			.CS = CD3DX12_SHADER_BYTECODE(csCSO),
			.NodeMask = 0,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC Material::CreatePSODesc(ID3DBlob* const vsCSO, ID3DBlob* const psCSO, ID3D12RootSignature* const rootSignature, DXGI_FORMAT rtvFormat, bool depthEnable, bool cubemap)
	{

		if (cubemap)
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
			{
				.pRootSignature = rootSignature,
				.VS = CD3DX12_SHADER_BYTECODE(vsCSO),
				.PS = CD3DX12_SHADER_BYTECODE(psCSO),
				.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
				.SampleMask = UINT_MAX,
				.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
				.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				.NumRenderTargets = 1,
				.RTVFormats = rtvFormat,
				.DSVFormat = {DXGI_FORMAT_D32_FLOAT},
				.SampleDesc
				{
					.Count = 1,
					.Quality = 0
				},
				.NodeMask = 0,
			};
			psoDesc.DepthStencilState =
			{
				.DepthEnable = depthEnable,
				.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
				.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
				.StencilEnable = FALSE,
			};

			return psoDesc;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
		{
			.pRootSignature = rootSignature,
			.VS = CD3DX12_SHADER_BYTECODE(vsCSO),
			.PS = CD3DX12_SHADER_BYTECODE(psCSO),
			.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
			.SampleMask = UINT_MAX,
			.RasterizerState =
			{
				.FillMode = D3D12_FILL_MODE_SOLID,
				.CullMode = D3D12_CULL_MODE_BACK,
				.FrontCounterClockwise = TRUE,
				.DepthBias = 0,
				.DepthBiasClamp = 0.0f,
				.SlopeScaledDepthBias = 0.0f,
				.DepthClipEnable = TRUE,
				.MultisampleEnable = FALSE,
				.AntialiasedLineEnable = FALSE,
				.ForcedSampleCount = 0,
				.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
			},
			.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
			.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			.NumRenderTargets = 1,
			.RTVFormats = rtvFormat,
			.DSVFormat = {DXGI_FORMAT_D32_FLOAT},
			.SampleDesc
			{
				.Count = 1,
				.Quality = 0
			},
			.NodeMask = 0,
		};
		psoDesc.DepthStencilState.DepthEnable = depthEnable;
		return psoDesc;
	}


	Material Material::CreateMaterial(ID3D12Device* const device, std::wstring_view vsFile, std::wstring_view psFile, std::wstring_view materialName, bool cubemap, DXGI_FORMAT format)
	{
		Microsoft::WRL::ComPtr<ID3DBlob> vs;
		Microsoft::WRL::ComPtr<ID3DBlob> ps;
		ThrowIfFailed(::D3DReadFileToBlob(vsFile.data(), &vs));
		ThrowIfFailed(::D3DReadFileToBlob(psFile.data(), &ps));


		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = CreatePSODesc(vs.Get(), ps.Get(), Material::BindlessRootSignature.Get(), format, true, cubemap);

		ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
		auto psoName = materialName.data() + std::wstring(L" PSO");
		pso->SetName(psoName.c_str());

		return { pso };
	}

	
	Material Material::CreateComputeMaterial(ID3D12Device* const device, std::wstring_view csFile, std::wstring_view materialName)
	{
		Microsoft::WRL::ComPtr<ID3DBlob> cs;
		ThrowIfFailed(::D3DReadFileToBlob(csFile.data(), &cs));

		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = CreateComputePSODesc(Material::BindlessRootSignature.Get(), cs.Get());
		ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
		auto name = materialName.data() + std::wstring(L" PSO");
		pso->SetName(name.c_str());

		return { pso };

	}
}