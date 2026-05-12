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
#include "ImGuizmo.h"

// We wrap the tracy functions in macros so that we can turn tracy on and off as necessary.
#if defined(TRACY_ENABLE)
	#include "tracy/public/tracy/Tracy.hpp"
	#define TRACY_PROFILER_COLOR_WAIT 0xff0000
	#define TRACY_PROFILER_COLOR_SUBMIT 0x0000ff
	#define TRACY_PROFILER_COLOR_PRESENT 0x00ff00
	#define TRACY_PROFILER_COLOR_CREATE 0xff6600
	#define TRACY_PROFILER_COLOR_DESTROY 0xffa500
	#define TRACY_PROFILER_COLOR_BARRIER 0xffffff
	#define TRACY_PROFILER_COLOR_CMD_DRAW 0x8b0000
	#define TRACY_PROFILER_COLOR_CMD_COPY 0x8b0a50
	#define TRACY_PROFILER_COLOR_CMD_RTX 0x8b0000
	#define TRACY_PROFILER_COLOR_CMD_DISPATCH 0x8b0000
	#define TRACY_PROFILER_FUNCTION() ZoneScoped
	#define TRACY_PROFILER_FUNCTION_COLOR(color) ZoneScopedC(color)
	#define TRACY_PROFILER_ZONE(name, color) \
	    {                                    \
	      ZoneScopedC(color);                \
	      ZoneName(name, strlen(name))
	#define TRACY_PROFILER_ZONE_END() }
	#define TRACY_PROFILER_THREAD(name) tracy::SetThreadName(name)
	#define TRACY_PROFILER_FRAME(name) FrameMarkNamed(name)
#else
	#define LVK_PROFILER_FUNCTION()
	#define LVK_PROFILER_FUNCTION_COLOR(color)
	#define LVK_PROFILER_ZONE(name, color) {
	#define LVK_PROFILER_ZONE_END() }
	#define LVK_PROFILER_THREAD(name)
	#define LVK_PROFILER_FRAME(name)
#endif


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



