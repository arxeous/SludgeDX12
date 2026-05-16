#include "utils.h"

namespace sludge::utils
{
	ID3D12CommandSignature* CreateCommandSignature(ID3D12Device* const device, const D3D12_COMMAND_SIGNATURE_DESC& desc, ID3D12RootSignature* rootSig)
	{
		assert(rootSig);
		ID3D12CommandSignature* cmdSig{ nullptr };
		ThrowIfFailed(device->CreateCommandSignature(&desc, rootSig, IID_PPV_ARGS(&cmdSig)));
		assert(cmdSig);
		return cmdSig;
	}
} // sludge::utils