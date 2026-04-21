#pragma once

#include "pch.h"
#include "utils/utils.h"

namespace sludge
{

	struct Vertex
	{
		DirectX::XMFLOAT3 Pos{};
		DirectX::XMFLOAT3 Normal{};
		DirectX::XMFLOAT2 TexC{};
		DirectX::XMFLOAT4 Tangent{};
	};

	// A structured buffer in DX12 is legit just an array of structures. Thats it. This plays nicely into use for
	// vertices and texture coordinates,as we can use a structured buffer to send in the data, and process in the shader randomly (i.e. random access)
	// Indices are still special though so we cant switch those to structured buffers yet.
	class StructuredBuffer
	{
	public:
		template<typename T>
		void CreateStructuredBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::span<const T> data, std::wstring_view name = L"Structured Buffer", D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
		{
			auto [buffer, intermediate] = utils::CreateGPUBuffer<T>(device, cmdList, data, flags);
			buffer_ = buffer;
			intermediate_ = intermediate;
			buffer_->SetName(name.data());
			std::wstring intermediateBufferName = name.data() + std::wstring(L" Intermediate Buffer");
			intermediate_->SetName(intermediateBufferName.c_str());
		}

		ID3D12Resource* const Buffer() const { return buffer_.Get(); };

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> buffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediate_;
	};
} // sludge