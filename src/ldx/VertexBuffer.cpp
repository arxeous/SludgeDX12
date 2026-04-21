#include "pch.h"
#include "VertexBuffer.h"

namespace sludge
{

	D3D12_VERTEX_BUFFER_VIEW VertexBuffer::VertexBufferView()
	{
		return bufferView_;
	}
} // sludge