#pragma once
#include "pch.h"
#include "utils/utils.h"
namespace sludge
{
	class VertexBuffer
	{
	public:
		template <typename T>
		void CreateVertexBuffer(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, std::span<const T> bufferData, std::wstring_view name);
		
		D3D12_VERTEX_BUFFER_VIEW VertexBufferView();

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateBuffer_;

		D3D12_VERTEX_BUFFER_VIEW bufferView_{};
	};

	template <typename T>
	void VertexBuffer::CreateVertexBuffer(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, std::span<const T> bufferData, std::wstring_view name)
	{
		auto [b, i] = utils::CreateGPUBuffer<T>(device, cmdList, bufferData);

		vertexBuffer_ = b;
		intermediateBuffer_ = i;

		bufferView_ =
		{
			.BufferLocation = vertexBuffer_->GetGPUVirtualAddress(),
			.SizeInBytes = static_cast<UINT>(bufferData.size_bytes()),
			.StrideInBytes = sizeof(T)
		};

		vertexBuffer_->SetName(name.data());
	}

} // slu