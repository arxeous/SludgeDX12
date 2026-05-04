#include "pch.h"
#include "DirectXClasses.h"
#include "utils/ReadData.h"
#include "utils/SceneUtils.h"

// Exports for the agility SDK
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 618; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }


namespace sludge
{

	DirectXContext::DirectXContext(Config& config) : IContext(config)
	{

	}

	void DirectXContext::Init()
	{
		InitRenderer();
		Load();
	}

	// Draw functions pretty much exactly like every other draw function for DX12 ever.
	// Go read frank luna for full details on how this works, but its just submitting draw commands to our 
	// gpu command queue really.
	void DirectXContext::Draw()
	{
		// Refresher: Allocators manage the memory for recording GPU commands
		// We have multiple allocators as we can not reset an allocator until all the commands its issued
		// have finished executing on the GPU. Multiple allocators means we dont stall.
		auto commandList_ = commandManager_.GetCommandList();
		Microsoft::WRL::ComPtr<ID3D12Resource> currBackBuffer = swapChainBuffer_[currentBackBufferIndex_];
		int selectedNode = -1;
		imGuiManager_.FrameStart();
		imGuiManager_.Begin("Scene Graph");
		ImGui::Separator();

		auto node = RenderSceneTreeUI(scene_, 0, selectedNode);
		if (node > -1) {
			selectedNode = node;
		}

		//for (auto& [name, model] : models_)
		//{
		//	model.UpdateFromUI(name.data(), cbModelPool_);
		//	auto viewProj = DirectX::XMMatrixMultiply(viewMatrix_, projMatrix_);
		//	model.UpdateData(viewProj, cbModelPool_);
		//}

		//ImGui::Begin("Material Data");
		//ImGui::SliderFloat4("Albedo", &matConstants_.DiffuseAlbedo.x, 0.0f, 1.0f);
		//ImGui::SliderFloat("Metalic Constant: ", &matConstants_.metalicness, 0.0, 1.0f);
		//ImGui::SliderFloat("Roughness Constant: ", &matConstants_.roughness, 0.0, 1.0f);
		//MaterialCB->CopyData(0, matConstants_);
		//ImGui::End();

		std::array<ID3D12DescriptorHeap*, 1> descriptorHeaps{ cbvSrvUavHeap_.GetDescriptorHeap() };
		commandList_->SetDescriptorHeaps(static_cast<uint32_t>(descriptorHeaps.size()), descriptorHeaps.data());

		// Since every shader uses the same root signature, we bind one and only change the PSO 
		// to switch out the shaders for the draw commands.
		Material::BindRootSignature(commandList_.Get());
		materials_[L"PBR Materials"].BindPSO(commandList_.Get());

		//auto helmetTexture = texturePool_.get(textures_[L"Helmet_Albedo"]);
		//commandList_->SetGraphicsRootDescriptorTable(4, helmetTexture->GPUDescriptorHandle());

		commandList_->RSSetViewports(1, &screenViewport_);
		commandList_->RSSetScissorRects(1, &scissorRect_);

		// Get our offscreen buffer ready to be drawn into.
		utils::TransitionResource(commandList_.Get(), offScreenRT_.Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv = offScreenRT_.RTVCPUDescriptorHandle();
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsv{ depthStencilBuffer_.BufferHandle() };


		static std::array<float, 4> clearColor{ 0.1f, 0.1f, 0.1f, 1.0f };
		imGuiManager_.SetClearColor(clearColor);
		commandList_->ClearRenderTargetView(rtv, clearColor.data(), 0, nullptr);
		commandList_->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		commandList_->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		auto passCB = cbPassPool_.get(passConstants[L"Test"]);

		auto viewProj = DirectX::XMMatrixMultiply(viewMatrix_, projMatrix_);
		updateModelData(modelData_, viewProj, cbModelPool_);
		RenderScene(commandList_.Get(), scene_, modelData_, 0);
		//ResourceIndices PBRResourceIDs
		//{
		//	.albedoID = 0,
		//	.roughnessID = 0,
		//	.VertexBufferID = 0,
		//	.passConstantID = passConstants[L"Test"].index(),
		//	.modelConstantID = 0,
		//	.IrradianceID = textures_[L"Irradiance Map UAV"].SRVHandle().index(),
		//	.PrefilterID = textures_[L"Prefiltered UAV"].SRVHandle().index(),
		//	.LutID = textures_[L"LUT UAV"].SRVHandle().index(),
		//	.NormalID = 0,
		//	.EmissiveID = 0,
		//	.AoID = 0,
		//};

		//// good old structured binding. Gotta love it for pairs n maps!
		//for (auto& [name, model] : models_)
		//{
		//	model.DrawNodes(commandList_.Get(), PBRResourceIDs);
		//}

		materials_[L"Skybox Material"].BindPSO(commandList_.Get());
		skybox_.UpdateData(viewProj, cbModelPool_);

		// The problem is that the rt index and the index of the other structured buffers (the first one) that hold the vertices are both 101. The latter overwrites the RT and so we get a 
		// byte stride mismatch. SOLUTION: Forgot to delete the model level vertex holder, which is no longer valid. So we change the function to retrieve the correct vertex.
		SkyBoxIndices skyboxIndices
		{
			.VertexBufferID = skybox_.VertexHolder().index(),
			.ModelConstantID = skybox_.ModelConstantHolder().index(),
			.TextureID = textures_[L"Skybox UAV"].SRVHandle().index()
		};
		commandList_->SetGraphicsRoot32BitConstants(0, 3, &skyboxIndices, 0);
		skybox_.Draw(commandList_.Get());


		utils::TransitionResource(commandList_.Get(), offScreenRT_.Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		utils::TransitionResource(commandList_.Get(), currBackBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// Final offscreen render
		materials_[L"Offscreen RT Material"].BindPSO(commandList_.Get());
		rtv = rtvHeap_.CPUDescriptorHandleStart();
		rtv.Offset(currentBackBufferIndex_, rtvHeap_.IncrementSize());

		commandList_->ClearRenderTargetView(rtv, clearColor.data(), 0, nullptr);
		commandList_->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		commandList_->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
		offScreenRT_.Bind(commandList_.Get());

		RTIndices rtIndices
		{
			.VertexBufferID = RenderTarget::VertexHolder().index(),
			.TextureID = RenderTarget::TextureHolder().index()
		};
		commandList_->SetGraphicsRoot32BitConstants(0, 2, &rtIndices, 0);
		commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// Since imgui gets it own heap, we need to set it before the call to frame end that will then do its draw call.
		std::array<ID3D12DescriptorHeap*, 1> imGuiDescriptorHeap{ imGuiSrvHeap_.GetDescriptorHeap() };
		commandList_->SetDescriptorHeaps(static_cast<uint32_t>(imGuiDescriptorHeap.size()), imGuiDescriptorHeap.data());

		imGuiManager_.End();
		imGuiManager_.FrameEnd(commandList_.Get());
		utils::TransitionResource(commandList_.Get(), currBackBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		frameCurrentFence_[currentBackBufferIndex_] = commandManager_.ExecuteCommandList(commandList_.Get());
		auto syncInterval = vSync_ ? 1u : 0u;
		//auto presentFlags = DXGI_PRESENT_ALLOW_TEARING;

		// Present updates the index of the back buffer we will use for the NEXT frame.
		ThrowIfFailed(swapChain_->Present(syncInterval, 0));

		// So when we use this function, we get the next back buffer to draw into . 
		currentBackBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();

		// What we are waiting on here is NOT for whether this frames commands are done executing, but if the frame we will be rendering into on the NEXT passes
		// instructions are done executing. At the very least were ensuring that the GPU is saturated by making the code stall out CPU side, which is what we want
		// in such cases.
		commandManager_.WaitForFenceValue(frameCurrentFence_[currentBackBufferIndex_]);
		frameIndex_++;
	}

	void DirectXContext::Update()
	{
		camera_.Update(d3dApp::GetTimer().DeltaTime());

		static DirectX::XMVECTOR rotationAxis = DirectX::XMVectorSet(0.0f, 1.0f, 1.0f, 0.0f);
		modelMatrix_ = DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f);
		viewMatrix_ = camera_.ViewMatrix();
		projMatrix_ = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(fov_), aspectRatio_, 0.1f, 1000.0f);

		DirectX::XMMATRIX mvpMatrix = DirectX::XMMatrixMultiply(modelMatrix_, viewMatrix_);
		// Set lighting data.
		auto passCB = cbPassPool_.get(passConstants[L"Test"]);
		passCB->ConstantBufferData().EyePosW = camera_.CamPos();
		passCB->ConstantBufferData().AmbientLight = { 0.4f, 0.4f, 0.4f, 1.0f };
		passCB->ConstantBufferData().Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
		passCB->ConstantBufferData().Lights[0].Strength = { 0.9f, 0.9f, 0.8f };
		passCB->UpdateBuffer();

	}

	void DirectXContext::Destroy()
	{
		commandManager_.FlushCommandQueue();
		imGuiManager_.Shutdown();
	}

	void DirectXContext::OnKeyAction(uint8_t keycode, bool isKeyDown)
	{
		if (isKeyDown)
		{
			if (keycode == VK_SPACE)
			{
				fov_ -= static_cast<float>(d3dApp::GetTimer().DeltaTime() * 10);
			}
		}
		camera_.HandleInput(keycode, isKeyDown);
	}

	void DirectXContext::OnResize()
	{
		if (width_ != d3dApp::ClientWidth() || height_ != d3dApp::ClientHeight())
		{
			// If we are resizing we need to change the swap chain buffers size, all of them.
			commandManager_.FlushCommandQueue();
			for (int i = 0; i < SwapChainBufferCount_; i++)
			{
				swapChainBuffer_[i].Reset();
				frameCurrentFence_[i] = frameCurrentFence_[currentBackBufferIndex_];
			}

			DXGI_SWAP_CHAIN_DESC desc{};
			ThrowIfFailed(swapChain_->GetDesc(&desc));
			ThrowIfFailed(swapChain_->ResizeBuffers(SwapChainBufferCount_, d3dApp::ClientWidth(), d3dApp::ClientHeight(), desc.BufferDesc.Format, desc.Flags));

			currentBackBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();

			width_ = d3dApp::ClientWidth();
			height_ = d3dApp::ClientHeight();

			// First thing we learn is that resize means recreation of all our stuff.
			CreateRTVs();
			CreateDSVs();
		}
	}

	void DirectXContext::InitRenderer()
	{
		EnableDebugLayer();
		CreateDXGIFactory();
		SelectAdapter();
		CreateDevice();
		commandManager_.CreateCommandManager(device_.Get());
		CreateSwapChain();
		// Swap chain count + 1 extra for the offscreen render target.
		rtvHeap_.CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, SwapChainBufferCount_ + 1, L"RTV Heap");
		dsvHeap_.CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1, L"DSV Heap");
		cbvSrvUavHeap_.CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 10000, L"CBV_SRV_UAV Heap");
		imGuiSrvHeap_.CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 64, L"ImGui Heap");
		CreateRTVs();
		CreateDSVs();


		ConstantBuffer<PassConstants> pass{};
		auto handle = cbPassPool_.create(std::move(pass));
		auto createdPassC = cbPassPool_.get(handle);
		passConstants[L"Test"] = Holder<PassConstantHandle>{handle};
		createdPassC->CreateConstantBuffer(device_.Get(), commandManager_.GetCommandList().Get(), PassConstants{}, cbvSrvUavHeap_, L"Pass Constants", passConstants[L"Test"].index());
		

		//PassCB = std::make_unique<UploadBuffer<PassConstants>>( device_.Get(), 1, true, L"Pass Constants");
		//MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(device_.Get(), 1, true, L"Material Constants");
		screenViewport_ =
		{
			.TopLeftX = 0.0f,
			.TopLeftY = 0.0f,
			.Width = static_cast<float>(width_),
			.Height = static_cast<float>(height_),
			.MinDepth = 0.0f,
			.MaxDepth = 1.0f
		};

		imGuiManager_.CreateImGuiRenderer(device_.Get(), commandManager_.GetCommandQueue().Get(), SwapChainBufferCount_, imGuiSrvHeap_);


	}

	void DirectXContext::Load()
	{
		// Reset the list and allocator before we start doing any work.
		auto commandList_ = commandManager_.GetCommandList();

		RenderTarget::InitBuffers(device_.Get(), commandList_.Get(), cbvSrvUavHeap_, geoPool_);
		offScreenRT_.CreateRenderTarget(device_.Get(), commandList_.Get(), DXGI_FORMAT_R16G16B16A16_FLOAT, rtvHeap_, cbvSrvUavHeap_, texturePool_, width_, height_, L"OffScreen Target");
		LoadMaterials();
		LoadModel(commandList_.Get());
		LoadTextures(commandList_.Get());
		std::thread IBLThread(std::bind(&DirectXContext::LoadIBLData, this));

		frameCurrentFence_[currentBackBufferIndex_] = commandManager_.ExecuteCommandList(commandList_.Get());
		commandManager_.FlushCommandQueue();

		IBLThread.join();
	}

	void DirectXContext::LoadIBLData()
	{
		// Run compute shader to get our cube map texture from our hdr.
		{
			auto command = commandManager_.GetCommandList();
			auto cmdList = command.Get();
			utils::TransitionResource(cmdList, textures_[L"Skybox UAV"].TextureResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			std::array<ID3D12DescriptorHeap*, 1> computeDescriptorHeaps
			{
				cbvSrvUavHeap_.GetDescriptorHeap()
			};

			cmdList->SetDescriptorHeaps(1u, computeDescriptorHeaps.data());
			materials_[L"Equirect Compute Material"].BindComputeShader(cmdList);

			CubeFromEquirectIndices CFE
			{
				.TextureID = textures_[L"HDR Test"].SRVHandle().index(),
				.OutputTextureID = textures_[L"Skybox UAV"].UAVHandle().index()
			};

			cmdList->SetComputeRoot32BitConstants(0, 8, &CFE, 0);

			// Recall that dispatch means were calling x * y *z groups total. The shader itself declares how large any given group will be.
			cmdList->Dispatch(SKYBOX_RESOLUTION / 32, SKYBOX_RESOLUTION / 32, 6);
			utils::TransitionResource(cmdList, textures_[L"Skybox UAV"].TextureResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
			commandManager_.ExecuteCommandList(cmdList);
			commandManager_.FlushCommandQueue();
		}
		

		{
			auto command = commandManager_.GetCommandList();
			auto cmdList = command.Get();
			// compute shader for irradiance map calculation
			std::array<ID3D12DescriptorHeap*, 1> computeDescriptorHeaps
			{
				cbvSrvUavHeap_.GetDescriptorHeap()
			};
			cmdList->SetDescriptorHeaps(1u, computeDescriptorHeaps.data());
			materials_[L"Irradiance Compute Material"].BindComputeShader(cmdList);
			IrradianceIndices II
			{
				.TextureID = textures_[L"Skybox UAV"].SRVHandle().index(),
				.OutputTextureID = textures_[L"Irradiance Map UAV"].UAVHandle().index()
			};
			// DX12 just does not play well straight up with legacy resource states like D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE in compute shaders.
			// Luckily for us common can acts as a srv for us when we need it in these contexts.

			utils::TransitionResource(cmdList, textures_[L"Irradiance Map UAV"].TextureResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			cmdList->SetComputeRoot32BitConstants(0, 8, &II, 0);
			cmdList->Dispatch(1, 1, 6);
			commandManager_.ExecuteCommandList(cmdList);
			commandManager_.FlushCommandQueue();
		}
		

		{
			auto command = commandManager_.GetCommandList();
			auto cmdList = command.Get();
			// Another compute shader to get the prefiltered env map aka radiance map. Both the compute shader for this and the irradiance map are very similar, with the only big difference being their means of sampling
			// vectors. i.e. importance vs uniform, specular vs diffuse.
			std::array<ID3D12DescriptorHeap*, 1> computeDescriptorHeaps
			{
				cbvSrvUavHeap_.GetDescriptorHeap()
			};
			cmdList->SetDescriptorHeaps(1u, computeDescriptorHeaps.data());
			materials_[L"Prefiltered Compute Material"].BindComputeShader(cmdList);
			PrefilteredMapIndices PI
			{
				.TextureID = textures_[L"Skybox UAV"].SRVHandle().index(),
				.OutputTextureID = textures_[L"Prefiltered UAV"].UAVHandle().index(),
				.Roughness = 0
			};
			utils::TransitionResource(cmdList, textures_[L"Prefiltered UAV"].TextureResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			cmdList->SetComputeRoot32BitConstants(0, 3, &PI, 0);
			cmdList->Dispatch(PREFILTERED_MAP_DIMENSION / 32, PREFILTERED_MAP_DIMENSION / 32, 6);
			// Generate the mip maps ! note that the mip maps for prefiltered env maps store roughness ! and are not for lod in textures. theryre used for ibl calculations.
			float maxMipLevel = std::log2(float(PREFILTERED_MAP_DIMENSION));
			for (float mipLevel = 0; mipLevel < maxMipLevel; ++mipLevel)
			{
				uint16_t mipWidth = PREFILTERED_MAP_DIMENSION >> uint16_t(mipLevel);
				float roughness = mipLevel / maxMipLevel;
				textures_[L"Prefiltered UAV"].CreateMipLevelTexture(device_.Get(), cbvSrvUavHeap_, texturePool_, mipLevel);
				PrefilteredMapIndices MipLevelI
				{
					.TextureID = textures_[L"Skybox UAV"].SRVHandle().index(),
					.OutputTextureID = textures_[L"Prefiltered UAV"].GetMipMapLevelIndex(mipLevel + 1),
					.Roughness = roughness
				};

				uint32_t threadGroupSize = std::max(1u, mipWidth / 32u);
				cmdList->SetComputeRoot32BitConstants(0, 3, &MipLevelI, 0);
				cmdList->Dispatch(threadGroupSize, threadGroupSize, 6);
			}
			commandManager_.ExecuteCommandList(cmdList);
			commandManager_.FlushCommandQueue();
		}


		{
			auto command = commandManager_.GetCommandList();
			auto cmdList = command.Get();
			std::array<ID3D12DescriptorHeap*, 1> computeDescriptorHeaps
			{
				cbvSrvUavHeap_.GetDescriptorHeap()
			};
			cmdList->SetDescriptorHeaps(1u, computeDescriptorHeaps.data());
			materials_[L"LUT Material"].BindComputeShader(cmdList);

			LutIndices LI
			{
				.TextureID = textures_[L"LUT UAV"].UAVHandle().index(),
			};
			utils::TransitionResource(cmdList, textures_[L"LUT UAV"].TextureResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			cmdList->SetComputeRoot32BitConstants(0, 1, &LI, 0);
			cmdList->Dispatch(LUT_DIMENSION / 32, LUT_DIMENSION / 32, 1);

			utils::TransitionResource(cmdList, textures_[L"Prefiltered UAV"].TextureResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			utils::TransitionResource(cmdList, textures_[L"Irradiance Map UAV"].TextureResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			utils::TransitionResource(cmdList, textures_[L"Skybox UAV"].TextureResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			utils::TransitionResource(cmdList, textures_[L"LUT UAV"].TextureResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			commandManager_.ExecuteCommandList(cmdList);
			commandManager_.FlushCommandQueue();
		}

	}

	void DirectXContext::LoadMaterials()
	{
		Material::CreateBindlessRootSignature(device_.Get(), L"../src/shaders/cso/vs.cso");
		materials_[L"PBR Materials"] = Material::CreateMaterial(device_.Get(), L"../src/shaders/cso/vs.cso", L"../src/shaders/cso/ps.cso", L"PBR Material");
		materials_[L"Skybox Material"] = Material::CreateMaterial(device_.Get(), L"../src/shaders/cso/sbVS.cso", L"../src/shaders/cso/sbPS.cso", L"Skybox Material", true);
		materials_[L"Irradiance Compute Material"] = Material::CreateComputeMaterial(device_.Get(), L"../src/shaders/cso/irradianceCompute.cso", L"Irradiance Compute Material");
		materials_[L"Equirect Compute Material"] = Material::CreateComputeMaterial(device_.Get(), L"../src/shaders/cso/skyboxCompute.cso", L"Equirect Compute Material");
		materials_[L"Prefiltered Compute Material"] = Material::CreateComputeMaterial(device_.Get(), L"../src/shaders/cso/prefilterCompute.cso", L"Prefiltered Compute Material");
		materials_[L"LUT Material"] = Material::CreateComputeMaterial(device_.Get(), L"../src/shaders/cso/lutCompute.cso", L"LUT Compute Material");
		materials_[L"Offscreen RT Material"] = Material::CreateMaterial(device_.Get(), L"../src/shaders/cso/osVS.cso", L"../src/shaders/cso/osPS.cso", L"OffScreen RT Material", false, DXGI_FORMAT_R8G8B8A8_UNORM);
	}

	void DirectXContext::LoadTextures(ID3D12GraphicsCommandList* cmdList)
	{
		int maxMipLevel = std::log2(float(PREFILTERED_MAP_DIMENSION));
		textures_[L"HDR Test"].CreateHDRTexture(device_.Get(), cmdList, cbvSrvUavHeap_,         "../assets/Environment/Environment.hdr", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, L"HDR Test", texturePool_);
		textures_[L"Skybox UAV"].CreateEmptyTexture(device_.Get(), cbvSrvUavHeap_, SKYBOX_RESOLUTION, SKYBOX_RESOLUTION, 6, 0, DXGI_FORMAT_R16G16B16A16_FLOAT, L"Skybox UAV", texturePool_, true);
		//textures_[L"Environment UAV"].CreateEmptyTexture(device_.Get(), cbvSrvUavHeap_, SKYBOX_RESOLUTION, SKYBOX_RESOLUTION, 6, 0, DXGI_FORMAT_R16G16B16A16_FLOAT, L"Environment UAV", texturePool_, true);
		// Since the irradiance map doesnt have a lot of high frequency details, its actually fine to just store it in a small 32x32 texture.
		textures_[L"Irradiance Map UAV"].CreateEmptyTexture(device_.Get(), cbvSrvUavHeap_, IRRADIANCE_MAP_DIMENSION, IRRADIANCE_MAP_DIMENSION, 6, 0, DXGI_FORMAT_R16G16B16A16_FLOAT, L"Irradiance Map UAV", texturePool_, true);
		textures_[L"Prefiltered UAV"].CreateEmptyTexture(device_.Get(), cbvSrvUavHeap_, PREFILTERED_MAP_DIMENSION, PREFILTERED_MAP_DIMENSION, 6, maxMipLevel, DXGI_FORMAT_R16G16B16A16_FLOAT, L"Prefiltered UAV", texturePool_, true);
		textures_[L"LUT UAV"].CreateEmptyTexture(device_.Get(), cbvSrvUavHeap_, LUT_DIMENSION, LUT_DIMENSION, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT, L"LUT UAV", texturePool_, true);
	}

	void DirectXContext::LoadModel(ID3D12GraphicsCommandList* cmdList)
	{
		//models_["Helmet"].LoadModel(device_.Get(), cmdList, cbvSrvUavHeap_, geoPool_, cbModelPool_, texturePool_, "../assets/DamagedHelmet/glTF/DamagedHelmet.gltf");
		//models_["Helmet"].GetTransformData().Scale = DirectX::XMFLOAT3(0.5, 0.5, 0.5);
		//models_["Helmet"].GetTransformData().Rotation = DirectX::XMFLOAT3(5, 0, 0);
		//auto viewProj = DirectX::XMMatrixMultiply(viewMatrix_, projMatrix_);
		//models_["Helmet"].UpdateData(viewProj, cbModelPool_);
		utils::loadMeshFile(device_.Get(), cmdList, cbvSrvUavHeap_, geoPool_, texturePool_, cbModelPool_, "../assets/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf", modelData_, scene_);
		auto viewProj = DirectX::XMMatrixMultiply(viewMatrix_, projMatrix_);
		modelData_.transform.Scale = DirectX::XMFLOAT3(0.1, 0.1, 0.1);
		modelData_.transform.Rotation = DirectX::XMFLOAT3(1.525, 0, 0);
		updateModelData(modelData_, viewProj, cbModelPool_);

		//models_["TestSpheres"].LoadModel(device_.Get(), cmdList, cbvSrvUavHeap_, geoPool_, cbModelPool_, texturePool_, "../assets/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf");
		//models_["TestSpheres"].GetTransformData().Scale = DirectX::XMFLOAT3(0.1, 0.1, 0.1);
		//models_["TestSpheres"].GetTransformData().Rotation = DirectX::XMFLOAT3(1.525, 0, 0);

		//models_["TestSpheres"].UpdateData(viewProj, cbModelPool_);

		skybox_.LoadModel(device_.Get(), cmdList, cbvSrvUavHeap_, geoPool_, cbModelPool_, texturePool_, "../assets/Cube/glTF/Cube.gltf");
		//skybox_.GetTransformData().Scale = DirectX::XMFLOAT3(0.5, 0.5, 0.5);
		//skybox_.GetTransformData().Rotation = DirectX::XMFLOAT3(5, 0, 0);
		skybox_.UpdateData(viewProj, cbModelPool_);
	}


	void DirectXContext::EnableDebugLayer()
	{
#ifdef DEBUG
		ThrowIfFailed(::D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface_)));
		debugInterface_->EnableDebugLayer();
		debugInterface_->SetEnableGPUBasedValidation(TRUE);
		debugInterface_->SetEnableSynchronizedCommandQueueValidation(TRUE);
		debugInterface_->SetGPUBasedValidationFlags(D3D12_GPU_BASED_VALIDATION_FLAGS_NONE);
#endif // DEBUG

	}

	void DirectXContext::SelectAdapter()
	{
		ThrowIfFailed(dxgiFactory_->EnumAdapterByGpuPreference(0u, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter_)));

#ifdef DEBUG
		DXGI_ADAPTER_DESC adapterDesc{};
		ThrowIfFailed(adapter_->GetDesc(&adapterDesc));
		std::wstring adapterInfo = L"Adapter Description : " + std::wstring(adapterDesc.Description) + L".\n";
		OutputDebugString(adapterInfo.c_str());
#endif
	}

	void DirectXContext::CreateDevice()
	{
		ThrowIfFailed(::D3D12CreateDevice(adapter_.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device_)));
#ifdef DEBUG
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(device_.As(&infoQueue)))
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

			// Configure queue filter to ignore info message severity.
			std::array<D3D12_MESSAGE_SEVERITY, 1> ignoreMessageSeverities
			{
				D3D12_MESSAGE_SEVERITY_INFO
			};

			std::array<D3D12_MESSAGE_ID, 1> ignoreMessageIDs
			{
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE
			};

			D3D12_INFO_QUEUE_FILTER infoQueueFilter
			{
				.DenyList
				{
					.NumSeverities = static_cast<UINT>(ignoreMessageSeverities.size()),
					.pSeverityList = ignoreMessageSeverities.data(),
					.NumIDs = static_cast<UINT>(ignoreMessageIDs.size()),
					.pIDList = ignoreMessageIDs.data()
				},
			};

			ThrowIfFailed(infoQueue->PushStorageFilter(&infoQueueFilter));
			ThrowIfFailed(device_->QueryInterface(IID_PPV_ARGS(&debugDevice_)));
		}
#endif // DEBUG
		device_->SetName(L"D3D12 Device");

	}

	void DirectXContext::CreateSwapChain()
	{


		DXGI_SWAP_CHAIN_DESC1 desc
		{
			.Width = width_,
			.Height = height_,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.Stereo = FALSE,
			.SampleDesc
			{
				.Count = 1,
				.Quality = 0,
			},
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = SwapChainBufferCount_,
			.Scaling = DXGI_SCALING_STRETCH,
			.SwapEffect = vSync_ ? DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL : DXGI_SWAP_EFFECT_FLIP_DISCARD,
			.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
			.Flags = 0
		};

		Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
		ThrowIfFailed(dxgiFactory_->CreateSwapChainForHwnd(commandManager_.GetCommandQueue().Get(), d3dApp::WindowHandle(), &desc, nullptr, nullptr, &swapChain));

		ThrowIfFailed(dxgiFactory_->MakeWindowAssociation(d3dApp::WindowHandle(), DXGI_MWA_NO_ALT_ENTER));

		ThrowIfFailed(swapChain.As(&swapChain_));

		currentBackBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();

	}

	void DirectXContext::CreateDXGIFactory()
	{
		UINT flags = 0;
#ifdef DEBUG
		flags = DXGI_CREATE_FACTORY_DEBUG;
#endif // DEBUG
		ThrowIfFailed(::CreateDXGIFactory2(flags, IID_PPV_ARGS(&dxgiFactory_)));
	}

	void DirectXContext::CreateRTVs()
	{
		D3D12_RENDER_TARGET_VIEW_DESC desc =
		{
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,


		};
		for (int i = 0; i < SwapChainBufferCount_; i++)
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
			const size_t buf_size = 10;
			char buf[buf_size]{};
			ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
			auto res = std::to_chars(buf, buf + buf_size, i);
			std::string str(buf, res.ptr - buf);
			auto wstr = StringToWString(str);
			backBuffer->SetName(wstr.data());
			device_->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHeap_.CPUDescriptorHandleEnd());
			rtvHeap_.OffsetHandles();
			swapChainBuffer_[i] = backBuffer;
			
		}
	}

	void DirectXContext::CreateDSVs()
	{
		depthStencilBuffer_.CreateDepthStencilBuffer(device_.Get(), dsvHeap_, width_, height_, L"Depth Stencil View");
	}

	void DirectXContext::RenderScene(ID3D12GraphicsCommandList* cmdList, const Scene& scene, ModelData& modelData, int node)
	{
		if (scene.meshForNode.find(node) != scene.meshForNode.end())
		{
			int meshIdx = scene.meshForNode.at(node);
			int materialIdx = scene.materialForNode.at(node);
			auto ibv = modelData.meshes[meshIdx].indexBuffer.IndexBufferView();
			cmdList->IASetIndexBuffer(&ibv);
			ResourceIndices PBRResourceIDs
			{
				.albedoID = !modelData.materials[materialIdx].albedo.empty() ? ModelData::loadedTextures[modelData.materials[materialIdx].albedo].SRVHandle().index() : 0,
				.roughnessID = !modelData.materials[materialIdx].metallicRoughnessMap.empty() ? ModelData::loadedTextures[modelData.materials[materialIdx].metallicRoughnessMap].SRVHandle().index() : 0,
				.VertexBufferID = modelData.meshes[meshIdx].vbHolder.index(),
				.passConstantID = passConstants[L"Test"].index(),
				.modelConstantID = modelData.cbHolder.index(),
				.IrradianceID = textures_[L"Irradiance Map UAV"].SRVHandle().index(),
				.PrefilterID = textures_[L"Prefiltered UAV"].SRVHandle().index(),
				.LutID = textures_[L"LUT UAV"].SRVHandle().index(),
				.NormalID = !modelData.materials[materialIdx].normalMap.empty() ? ModelData::loadedTextures[modelData.materials[materialIdx].normalMap].SRVHandle().index() : 0,
				.EmissiveID = !modelData.materials[materialIdx].emissiveMap.empty() ? ModelData::loadedTextures[modelData.materials[materialIdx].emissiveMap].SRVHandle().index() : 0,
				.AoID = !modelData.materials[materialIdx].aoMap.empty() ? ModelData::loadedTextures[modelData.materials[materialIdx].aoMap].SRVHandle().index() : 0,
			};
			cmdList->SetGraphicsRoot32BitConstants(0, 11, &PBRResourceIDs, 0);
			cmdList->DrawIndexedInstanced(modelData.meshes[meshIdx].indexCount, 1, 0, 0, 0);
		}
		
		for (int child = scene.hierarchy[node].firstChild; child != -1; child = scene.hierarchy[child].nextSibling)
		{
			RenderScene(cmdList, scene, modelData, child);
		}
	}

	int DirectXContext::RenderSceneTreeUI(const Scene& scene, int node, int selectedNode)
	{

		int strID = scene.nameForNode.contains(node) ? scene.nameForNode.at(node) : -1;
		const std::string name = strID > -1 ? scene.nodeNames[strID] : std::string{};
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
} // sludge