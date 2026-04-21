#pragma once
#include "pch.h"

namespace sludge
{
	class IndexBuffer
	{
	public:
		void CreateIndexBuffer(ID3D12Device* const device, ID3D12GraphicsCommandList* cmdList, std::span<const uint32_t> bufferData, std::wstring_view name);
		D3D12_INDEX_BUFFER_VIEW IndexBufferView() { return bufferView_; };

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateBuffer_;

		D3D12_INDEX_BUFFER_VIEW bufferView_{};
	};

} // sludge