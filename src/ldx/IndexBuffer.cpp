#include "IndexBuffer.h"
#include "pch.h"
#include "utils/utils.h"

namespace sludge
{
	void IndexBuffer::CreateIndexBuffer(ID3D12Device* const device, ID3D12GraphicsCommandList* cmdList, std::span<const uint32_t> bufferData, std::wstring_view name)
	{
		auto [b, i] = utils::CreateGPUBuffer<uint32_t>(device, cmdList, bufferData);

		// My guess is that ComPtrs have a move assignment that puts the pointer into the underlying pointer for ComPtr without much fuss.
		// At the very least theres a 
		indexBuffer_ = b;
		intermediateBuffer_ = i;

		bufferView_ =
		{
			.BufferLocation = indexBuffer_->GetGPUVirtualAddress(),
			.SizeInBytes = static_cast<UINT>(bufferData.size_bytes()),
			.Format = DXGI_FORMAT_R32_UINT
		};
		indexBuffer_->SetName(name.data());

	}


}

