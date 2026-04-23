#pragma once

#include "pch.h"
#include "IContext.h"
#include "d3dApp.h"



// Alot of our initial set up mimics the set up for lvk,lightweight vulkan, because I really like 
// the Vulkan 3D graphics Rendering Cookbook despite the fact that I'm writing this whole thing in DX12.
// its a good book for expert level techniques that you dont get to see everyday.
namespace sludge
{
#define MaxLights 16
	// Our basic lighting model is going to be Frank Luna BlinnPhong, modified to make things simpler. Nothing crazy for now. We will move onto PBR techniques soon enough.
	struct Light
	{
		DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };
		float FalloffStart = 1.0f;                          // point/spot light only
		DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spot light only
		float FalloffEnd = 10.0f;                           // point/spot light only
		DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only
		float SpotPower = 64.0f;                            // spot light only
	};

	struct alignas(256) PassConstants
	{
		DirectX::XMVECTOR EyePosW{ 0.0f, 0.0f, 0.0f, 0.0f };
		DirectX::XMVECTOR AmbientLight{ 0.0f, 0.0f, 0.0f, 1.0f };

		Light Lights[MaxLights];
	};

	struct alignas(256) MaterialConstants
	{
		DirectX::XMFLOAT4 DiffuseAlbedo{ 1.0f, 1.0f, 1.0f, 1.0f };
		float metalicness{0.5f};
		float roughness{0.5f};
		DirectX::XMFLOAT3 padding;
	};


	class DirectXContext : public IContext
	{
	public:
		DirectXContext(Config& config);

		virtual void Init() override;
		virtual void Draw() override;
		virtual void Update() override;
		virtual void Destroy() override;

		// Functions that are called in conjunction with the windows app code.
		virtual void OnKeyAction(uint8_t keycode, bool isKeyDown) override;
		virtual void OnResize() override;

	private:
		void InitRenderer();
		void Load();
		void LoadMaterials();
		void LoadTextures(ID3D12GraphicsCommandList* cmdList);
		void LoadModel(ID3D12GraphicsCommandList* cmdList);
		void LoadCubeMap(ID3D12GraphicsCommandList* cmdList);
		void EnableDebugLayer();
		void SelectAdapter();
		void CreateDevice();
		void CreateSwapChain();
		void CreateDXGIFactory();
		void CreateRTVs();
		void CreateDSVs();

		static constexpr uint32_t SKYBOX_RESOLUTION = 1024;
		static constexpr uint32_t IRRADIANCE_MAP_DIMENSION = 32;
		static constexpr uint32_t PREFILTERED_MAP_DIMENSION = 512;
		static constexpr uint16_t ROOT_SIG_CONSTANT_COUNT = 64;
		static constexpr uint8_t SwapChainBufferCount_ = 3;
		static constexpr uint32_t LUT_DIMENSION = 512;
		Microsoft::WRL::ComPtr<ID3D12Device8> device_;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter_;
		Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
		// SwapChain / presentation control objects.
		Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, SwapChainBufferCount_> swapChainBuffer_{};
		std::array<uint64_t, SwapChainBufferCount_> frameCurrentFence_{};
		bool vSync_{ true };
		uint32_t currentBackBufferIndex_{};

		// Debug message setters
		Microsoft::WRL::ComPtr<ID3D12Debug3> debugInterface_;
		Microsoft::WRL::ComPtr<ID3D12DebugDevice2> debugDevice_;
		//Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer_;

		DescriptorHeap rtvHeap_;
		DescriptorHeap dsvHeap_;
		DescriptorHeap cbvSrvUavHeap_;
		DescriptorHeap imGuiSrvHeap_;

		RenderTarget offScreenRT_{};
		DepthStencilBuffer depthStencilBuffer_{};
		VertexBuffer rtVB_;
		IndexBuffer  rtIB_;

		D3D12_VIEWPORT screenViewport_;
		D3D12_RECT scissorRect_{ .left = 0, .top = 0, .right = LONG_MAX, .bottom = LONG_MAX };

		float fov_{ 45.0f };
		DirectX::XMMATRIX modelMatrix_{};
		DirectX::XMMATRIX viewMatrix_{};
		DirectX::XMMATRIX projMatrix_{};

		Camera camera_{};

		CommandManager commandManager_{};
		ImGuiRenderer imGuiManager_{};

		//MaterialConstants matConstants_{};
		//PassConstants passConstants_{};
		std::map<std::string_view, Model> models_;
		Model skybox_{};
		utils::Pool<GeometryTag, StructuredBuffer> geoPool_{};

		std::map<std::wstring_view, Texture> textures_;
		utils::Pool<TextureTag, DescriptorHandle> texturePool_{};
		
		std::map<std::wstring_view, Holder<PassConstantHandle>> passConstants;
		utils::Pool<PassConstantTag,  ConstantBuffer<PassConstants>> cbPassPool_;
		utils::Pool<ModelConstantTag, ConstantBuffer<ModelConstants>> cbModelPool_;

		std::map<std::wstring_view, Material> materials_;
		
		//std::unique_ptr<UploadBuffer<PassConstants>> PassCB{};
		//std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB{};

	};

	
}