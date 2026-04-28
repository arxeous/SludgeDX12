#include "pch.h"

#include "core/d3dApp.h"
#include "ImGuiRenderer.h"

namespace sludge
{
	void ImGuiRenderer::CreateImGuiRenderer(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, uint32_t fIF, DescriptorHeap& descriptor)
	{
		IMGUI_CHECKVERSION();

		ImGui::CreateContext();
		auto& io = ImGui::GetIO();
		(void)io;
		io.Fonts->Build();
		
		ImGui::StyleColorsDark();

		ImGui_ImplWin32_EnableDpiAwareness();
		ImGui_ImplWin32_Init(d3dApp::WindowHandle());

		//ImGui_ImplDX12_InitInfo info = {};
		//info.Device = device;
		//info.CommandQueue = cmdQueue;
		//info.NumFramesInFlight = fIF;
		//info.SrvDescriptorHeap = descriptor.GetDescriptorHeap();
		//info.SrvDescriptorAllocFn = nullptr;
		//info.SrvDescriptorFreeFn = nullptr;
		//info.LegacySingleSrvCpuDescriptor.ptr = 0;
		//info.LegacySingleSrvGpuDescriptor.ptr = 0;

		//bool success = ImGui_ImplDX12_Init(&info);
		bool success =  ImGui_ImplDX12_Init(device, fIF, DXGI_FORMAT_R8G8B8A8_UNORM, descriptor.GetDescriptorHeap(), descriptor.CPUDescriptorHandleStart(), descriptor.GPUDescriptorHandleStart());
		if (success)
		{
			printf("Claims to have texture");
		}
		else
		{
			printf("Does not");
		}

	}
	void ImGuiRenderer::Shutdown()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
	void ImGuiRenderer::FrameStart()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}
	void ImGuiRenderer::FrameEnd(ID3D12GraphicsCommandList* cmdList)
	{
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
	}

	void ImGuiRenderer::Begin(std::string_view name)
	{
		// again, im not the most comfortable with using string views where null terminated strings should go, but since names and such will be created using strings
		// which ARE null terminated, we roll with it for now.
		ImGui::Begin(name.data());
	}

	void ImGuiRenderer::End()
	{
		ImGui::End();
	}

	void ImGuiRenderer::SetClearColor(std::span<float> clearColor)
	{
		if (ImGui::TreeNode("Clear Color"))
		{
			ImGui::ColorEdit3("Clear Color", clearColor.data());
			ImGui::TreePop();
		}
	}
}