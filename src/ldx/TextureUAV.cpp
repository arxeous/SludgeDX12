#include "pch.h"
#include "TextureUAV.h"

namespace sludge
{
	void TextureUAV::CreateUAVFromTexture(ID3D12Device* device, DescriptorHeap& heap, Texture& texture, std::wstring_view name, int index)
	{
		width_ = texture.Width();
		height_ = texture.Height();


	}

	ID3D12Resource* TextureUAV::TextureResource() const
	{
		return nullptr;
	}

} // sludge