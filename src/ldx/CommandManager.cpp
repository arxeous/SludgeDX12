#include "pch.h"
#include "CommandManager.h"

namespace sludge
{
	void sludge::CommandManager::CreateCommandManager(ID3D12Device* const device, D3D12_COMMAND_LIST_TYPE commandListType, std::wstring_view name)
	{
		device_ = device;
		commandListType_ = commandListType;
		D3D12_COMMAND_QUEUE_DESC desc
		{
			.Type = commandListType_,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0
		};

		ThrowIfFailed(device_->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue_)));
		commandQueue_->SetName(name.data());

		// Its virtually impossible to execute commands without the fence sync object.
		ThrowIfFailed(device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));
		std::wstring fenceName = name.data() + std::wstring(L" Fence");
		fence_->SetName(fenceName.data());

		fenceEvent_ = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!fenceEvent_)
		{
			ErrorMessage(L"Failed to create fence event.");
		}
	}
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandManager::GetCommandList()
	{
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

		// To get a list, we look through our queue of allocators and see which one isnt in use.
		// If we dont have any, create a new one!
		if (!commandAllocatorQueue_.empty() && IsFenceComplete(commandAllocatorQueue_.front().fenceValue))
		{
			commandAllocator = commandAllocatorQueue_.front().commandAllocator;
			commandAllocatorQueue_.pop();

			ThrowIfFailed(commandAllocator->Reset());
		}
		else
		{
			commandAllocator = CreateCommandAllocator();
		}

		if (!commandListQueue_.empty())
		{
			commandList = commandListQueue_.front();
			commandListQueue_.pop();

			ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
		}
		else
		{
			commandList = CreateCommandList(commandAllocator.Get());
		}

		// apparently you can freely create associations between the list and the allocator,
		// doing this will then allows you to just check which allocator this list was allocated from 
		ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

		return commandList;
	}

	uint64_t CommandManager::ExecuteCommandList(ID3D12GraphicsCommandList* const cmdList)
	{
		ThrowIfFailed(cmdList->Close());
		ID3D12CommandAllocator* commandAllocator{ nullptr };
		UINT dataSize = sizeof(ID3D12CommandAllocator);

		// heres where that association comes in. This way we never have to manually go through the queue,
		// we can always just retrieve the appropriate allocator associate with this list.
		ThrowIfFailed(cmdList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

		std::array<ID3D12CommandList* const, 1> commandLists{ cmdList };
		commandQueue_->ExecuteCommandLists(static_cast<UINT>(commandLists.size()), commandLists.data());

		uint64_t fenceValue = Signal();

		// emplace just calls emplace_back and we already know how that works. really no different here.
		commandAllocatorQueue_.emplace(CommandAllocator{ .fenceValue = fenceValue, .commandAllocator = commandAllocator });
		commandListQueue_.emplace(cmdList);

		// Manual reference count handling, gross.
		commandAllocator->Release();

		return fenceValue;
	}
	uint64_t CommandManager::Signal()
	{
		uint64_t fenceForSignal = ++fenceValue_;
		ThrowIfFailed(commandQueue_->Signal(fence_.Get(), fenceForSignal));

		return fenceForSignal;
	}

	bool CommandManager::IsFenceComplete(uint64_t fenceValue)
	{
		return fence_->GetCompletedValue() >= fenceValue;
	}

	void CommandManager::WaitForFenceValue(uint64_t fenceValue)
	{
		if (!IsFenceComplete(fenceValue))
		{
			ThrowIfFailed(fence_->SetEventOnCompletion(fenceValue, fenceEvent_));
			::WaitForSingleObject(fenceEvent_, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
		}
	}
	void CommandManager::FlushCommandQueue()
	{
		uint64_t fenceValue = Signal();
		WaitForFenceValue(fenceValue);
	}
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandManager::CreateCommandAllocator()
	{
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
		ThrowIfFailed(device_->CreateCommandAllocator(commandListType_, IID_PPV_ARGS(&commandAllocator)));

		return commandAllocator;
	}
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandManager::CreateCommandList(ID3D12CommandAllocator* commandAllocator)
	{
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;
		ThrowIfFailed(device_->CreateCommandList(0, commandListType_, commandAllocator, nullptr, IID_PPV_ARGS(&cmdList)));

		return cmdList;
	}
}