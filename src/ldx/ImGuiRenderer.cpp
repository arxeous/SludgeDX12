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

		bool success =  ImGui_ImplDX12_Init(device, fIF, DXGI_FORMAT_R8G8B8A8_UNORM, descriptor.GetDescriptorHeap(), descriptor.ImGuiCPUDescriptorHandleStart(), descriptor.ImGuiGPUDescriptorHandleStart());
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

	void ImGuiRenderer::RenderEditNodeUI(Scene& scene, ModelData& modelData, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX proj, int node, int& outUpdateMaterialIndex,
		utils::Pool<utils::TextureTag, DescriptorHandle>& pool)
	{
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::BeginFrame();

		std::string name = GetNodeName(scene, node);
		std::string label = name.empty() ? (std::string("Node") + std::to_string(node)) : name;
		label = "Node: " + label;

		if (const ImGuiViewport* v = ImGui::GetMainViewport())
		{
			ImGui::SetNextWindowPos(ImVec2(v->WorkSize.x * 0.83f, 100));
			ImGui::SetNextWindowSize(ImVec2(v->WorkSize.x / 6, v->WorkSize.y - 210));
		}
		ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
		if (!name.empty())
		{
			ImGui::Text("%s", label.c_str());
		}

		if (node >= 0)
		{
			ImGui::Separator();
			ImGui::Text("%s", "Material");
			EditMaterialUI(scene, modelData, node, outUpdateMaterialIndex, ModelData::loadedTextures, pool);
			// Editing for the mesh and the texture/material are to go in this portion of code
			//ImGuizmo::PopID();
		}


		ImGui::End();
	}

	int ImGuiRenderer::RenderSceneTreeUI(const Scene& scene, int node, int selectedNode)
	{
		const std::string name = GetNodeName(scene, node);
		const std::string label = name.empty() ? (std::string("Node") + std::to_string(node)) : name;
		const bool isLeaf = scene.hierarchy[node].firstChild < 0;
		ImGuiTreeNodeFlags flags = isLeaf ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet : 0;
		if (node == selectedNode)
		{
			flags |= ImGuiTreeNodeFlags_Selected;
		}
		ImVec4 color = isLeaf ? ImVec4(0, 1, 0, 1) : ImVec4(1, 1, 1, 1);
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		const bool isOpened = ImGui::TreeNodeEx(&scene.hierarchy[node], flags, "%s", label.c_str());
		ImGui::PopStyleColor();
		ImGui::PushID(node);
		if (ImGui::IsItemHovered() && isLeaf)
		{
			selectedNode = node;
		}
		if (isOpened)
		{
			for (int child = scene.hierarchy[node].firstChild; child != -1; child = scene.hierarchy[child].nextSibling)
			{
				if (int subNode = RenderSceneTreeUI(scene, child, selectedNode); subNode > -1)
				{
					selectedNode = subNode;
				}
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
		return selectedNode;
	}

	bool ImGuiRenderer::EditMaterialUI(Scene& scene, ModelData& modelData, int node, int& outUpdateMaterialIndex, std::map<std::string_view, Texture>& textureCache,
		utils::Pool<utils::TextureTag, DescriptorHandle>& pool)
	{
		if (!scene.materialForNode.contains(node))
			return false;
		const uint32_t materialIdx = scene.materialForNode[node];
		MeshMaterial& material = modelData.materials[materialIdx];

		bool updated = false;

		//updated |= ImGui::ColorEdit3("Base color", );

		const char* TextureGalleryName = "Texture Gallery";

		auto drawTextureUI = [&textureCache, TextureGalleryName, &pool](const char* name, std::string& texture)
			{
				if (texture.empty())
				{
					return -1;
				}
				ImGui::Text("%s", name);
				auto srvHandle = pool.get(textureCache.at(texture).SRVHandle());
				ImGui::Image((ImTextureID)srvHandle->GpuDescriptorHandle.ptr, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
				if (ImGui::IsItemClicked())
				{
					ImGui::OpenPopup(TextureGalleryName);
				}
			};

		ImGui::Separator();
		ImGui::Text("Edit Texture");
		ImGui::Separator();

		drawTextureUI("Base Texture", material.albedo);
		drawTextureUI("Normal Texture", material.normalMap);
		drawTextureUI("Metallic Roughness Texture", material.metallicRoughnessMap);
		drawTextureUI("Emissive Texture", material.emissiveMap);
		drawTextureUI("AO Texture", material.aoMap);


		if (const ImGuiViewport* v = ImGui::GetMainViewport())
		{
			ImGui::SetNextWindowPos(ImVec2(v->WorkSize.x * 0.5f, v->WorkSize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		}
	
		if (ImGui::BeginPopupModal(TextureGalleryName, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			int i = 0;
			for (auto& [name, texture] : textureCache)
			{
				if (i && i % 4 != 0)
				{
					ImGui::SameLine();
				}
				auto srvHandle = pool.get(texture.SRVHandle());
				ImGui::Image((ImTextureID)srvHandle->GpuDescriptorHandle.ptr, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
				if (ImGui::IsItemHovered())
				{
					ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0x66ffffff);
				}
				if (ImGui::IsItemClicked())
				{
					updated = true;
					ImGui::CloseCurrentPopup();
				}
				i++;
			}
			ImGui::EndPopup();
		}
		if (updated)
		{
			outUpdateMaterialIndex = static_cast<int>(materialIdx);
		}

		return updated;
	}
}