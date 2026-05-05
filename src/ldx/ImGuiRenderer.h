#pragma once 

#include "pch.h"
#include "DescriptorHeap.h"
#include "Scene.h"
#include "utils/ModelData.h"
#include "utils/Pool.h"
#include "utils/Holder.h"
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

		void RenderEditNodeUI(Scene& scene, ModelData& modelData, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX proj, int node, int& outUpdateMaterialIndex,
			utils::Pool<utils::TextureTag, DescriptorHandle>& pool);
		int RenderSceneTreeUI(const Scene& scene, int node, int selectedNode);
	private:
		bool EditMaterialUI(Scene &scene, ModelData& modelData, int node, int& outUpdateMaterialIndex, std::map<std::string_view, Texture>& textureCache,
			utils::Pool<utils::TextureTag, DescriptorHandle>& pool);
	};
}