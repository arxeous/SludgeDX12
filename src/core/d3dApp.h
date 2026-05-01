#pragma once
#include "pch.h"
#include "utils/Timer.h"
#include "utils/Camera.h"
#include "utils/utils.h"
#include "utils/Pool.h"
#include "utils/Handle.h"
#include "utils/Holder.h"
#include "ldx/DescriptorHeap.h"
#include "ldx/VertexBuffer.h"
#include "ldx/IndexBuffer.h"
#include "ldx/Model.h"
#include "ldx/CommandManager.h"
#include "ldx/Texture.h"
#include "ldx/UploadBuffer.h"
#include "ldx/ImGuiRenderer.h"
#include "ldx/RenderTarget.h"
#include "ldx/CubeMap.h"
#include "ldx/Material.h"
#include "ldx/DepthStencilBuffer.h"
#include "ldx/Scene.h"
#include "utils/ModelData.h"

namespace sludge
{
	// Gotta forward declare this baby.
	class IContext;

	class d3dApp
	{
	public:
		static int	    Run(IContext* ctx, HINSTANCE instance);
		static inline HWND		WindowHandle() { return windowHandle_; };
		static inline uint32_t ClientWidth() { return clientWidth_; };
		static inline uint32_t ClientHeight() { return clientHeight_; };
		static inline RECT& WindowRect() { return rect_; };
		static inline Timer& GetTimer() { return timer_; };

		static double DeltaTime();
		static double TotalTime();


	private:
		// In order to actually create a windows window we need a message processing function to callback to.
		static LRESULT CALLBACK WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

		static void ToggleFullScreen();
		// string_views give us a way to pass read only expection safe non owning handles 
		static constexpr auto windowName_{ L"Base Window Class" };
		static inline HWND		windowHandle_{};
		static inline uint32_t	clientWidth_{};
		static inline uint32_t	clientHeight_{};
		static inline Timer		timer_{};
		static inline RECT		rect_{};
		static inline bool		fullscreen_{ false };
		
	};
} // namespace



