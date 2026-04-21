#pragma once 

#include "pch.h"
#include "DescriptorHeap.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

namespace sludge
{
	class ImGuiRenderer
	{
	public:
		void CreateImGuiRenderer(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, uint32_t fIF, DescriptorHeap& descriptor);
		void Shutdown();

		void FrameStart();
		void FrameEnd(ID3D12GraphicsCommandList* cmdList);

		void Begin(std::string_view name);
		void End();

		void SetClearColor(std::span<float> clearColor);
	};
}