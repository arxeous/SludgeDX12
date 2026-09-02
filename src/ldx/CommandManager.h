#pragma once
#include "pch.h"
#include "UploadBuffer.h"
#include "utils/utils.h"
namespace sludge
{
	struct PBRCommandArgumentBuffer
	{
		utils::ResourceIndices indices;
		D3D12_DRAW_INDEXED_ARGUMENTS drawArgs;
	};
	struct SkyBoxCommandArgumentBuffer
	{
		utils::SkyBoxIndices indices;
		D3D12_DRAW_INDEXED_ARGUMENTS drawArgs;
	};
	struct OffscreenCommandArgumentBuffer
	{
		utils::RTIndices indices;
		D3D12_DRAW_INDEXED_ARGUMENTS drawArgs;
	};
	struct CommandAllocator
	{
		uint64_t fenceValue{};
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	};
	// The command manager essentially aims to abstract the need for a manual buffer with indices pointing to the allocator in our DX12 class code so that new
	// commands may be recorded. 
	// This is facilitated by the use of a queue for both allocators and buffers, and mainly through the GetCommandList and ExectureCommandList functions
	class CommandManager
	{
	public:
		void CreateCommandManager(ID3D12Device* const device, D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring_view name = L"Main Queue");

		[[nodiscard]]
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList();

		[[nodiscard]]
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const { return commandQueue_; };

		[[nodiscard]]
		uint64_t ExecuteCommandList(ID3D12GraphicsCommandList* const cmdList);

		[[nodiscard]]
		uint64_t Signal();
		void CreateCommandSignature(ID3D12Device* const device, uint32_t rootSigID, ID3D12RootSignature* rootSig);

		bool IsFenceComplete(uint64_t fenceValue);
		void WaitForFenceValue(uint64_t fenceValue);
		void FlushCommandQueue();

		// To use indirect drawing, we need a command signature for every root signature used in our engine. Given that we are on a bindless model
		// that only ever needs a single root signature, we initialize these command signatures all with the same bindless signature. 
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> pbrCommandSignature_;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> skyBoxCommandSignature_;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> offScreenCommandSignature_;

		std::shared_ptr<UploadBuffer<PBRCommandArgumentBuffer>> pbrCommandArgBuffer_;
		std::shared_ptr<UploadBuffer<SkyBoxCommandArgumentBuffer>> skyBoxCommandArgBuffer_;
		std::shared_ptr<UploadBuffer<OffscreenCommandArgumentBuffer>> offScreenCommandArgBuffer_;

	private:
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ID3D12CommandAllocator* commandAllocator);

		D3D12_COMMAND_LIST_TYPE commandListType_{ D3D12_COMMAND_LIST_TYPE_DIRECT };

		// To save on having to send in a device in the parameter everytime we want to execute or wait for signals etc, we store a reference
		// to our device.
		Microsoft::WRL::ComPtr<ID3D12Device> device_;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;

		Microsoft::WRL::ComPtr<ID3D12Fence> fence_;

		HANDLE fenceEvent_{};
		uint64_t fenceValue_{};

		// We have a queue of allocators and lists, precicely for the reason weve always had multiple allocators!
		// So that we can issue commands while the GPU is processing commands for previous frames. 
		std::queue<CommandAllocator> commandAllocatorQueue_{};
		std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> commandListQueue_{};


	};
} // sludge