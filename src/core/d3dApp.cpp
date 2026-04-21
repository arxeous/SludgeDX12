#include "pch.h"

#include "d3dApp.h"
#include "IContext.h"
#include "utils/Timer.h"
#include "backends/imgui_impl_win32.h"

//

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
namespace sludge 
{
	LRESULT CALLBACK d3dApp::WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
	{
		IContext* ctx = reinterpret_cast<IContext*>(GetWindowLongPtr(windowHandle, GWLP_USERDATA));

		if (ImGui_ImplWin32_WndProcHandler(windowHandle, message, wParam, lParam))
		{
			return true;
		}

		switch (message)
		{
			case WM_CREATE:
			{
				LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
				SetWindowLongPtr(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
				break;
			}

			case WM_KEYDOWN:
			{
				ctx->OnKeyAction(static_cast<uint8_t>(wParam), true);


				if (wParam == VK_ESCAPE)
				{
					::DestroyWindow(windowHandle_);
				}

				if (wParam == VK_F11)
				{
					ToggleFullScreen();
				}

				break;
			}

			case WM_KEYUP:
			{
				ctx->OnKeyAction(static_cast<uint8_t>(wParam), false);
				break;
			}

			case WM_SIZE:
			{
				if (fullscreen_)
				{
					::GetClientRect(windowHandle_, &rect_);
					std::tie<uint32_t, uint32_t>(clientWidth_, clientHeight_) = utils::ClientRegionDimensions(rect_);
				}
				break;
			}

			case WM_DESTROY:
			{
				::PostQuitMessage(0);
				break;
			}

			default:
			{
				break;
			}
		}

		return ::DefWindowProc(windowHandle, message, wParam, lParam);
	}

	// This is more windows API code than it is DX12 but its always good to know how to do it.
	void d3dApp::ToggleFullScreen()
	{
		if (fullscreen_)
		{
			::GetWindowRect(windowHandle_, &rect_);
			UINT style = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

			::SetWindowLong(windowHandle_, GWL_STYLE, style);
			auto monitor = MonitorFromWindow(windowHandle_, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEXW info{};
			::GetMonitorInfoW(monitor, &info);
			info.cbSize = sizeof(MONITORINFOEXW);
			auto [w, h] = utils::UserMonitorDimensions(info);
			::SetWindowPos(windowHandle_, HWND_TOP, info.rcMonitor.left, info.rcMonitor.top, w, h, SWP_FRAMECHANGED | SWP_NOACTIVATE);
			::ShowWindow(windowHandle_, SW_MAXIMIZE);
		}
		else
		{
			::SetWindowLong(windowHandle_, GWL_STYLE, WS_OVERLAPPEDWINDOW);
			std::tie<uint32_t, uint32_t>(clientWidth_, clientHeight_) = utils::ClientRegionDimensions(rect_);

			::SetWindowPos(windowHandle_, HWND_NOTOPMOST, rect_.left, rect_.right, clientWidth_, clientHeight_, SWP_FRAMECHANGED | SWP_NOACTIVATE);
			::ShowWindow(windowHandle_, SW_NORMAL);
		}

		fullscreen_ = !fullscreen_;

	}

	int d3dApp::Run(IContext* ctx, HINSTANCE instance)
	{
		// Setting up window application. 
		// Most of this is boiler plate you can skim from reading Frank Lunas book on DX12, shoutout to that text, 
		// good place to get a working grasp on DX12 for beginners.
        
        WNDCLASSEXW wc
        {
            .cbSize = sizeof(WNDCLASSEXW),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = WindowProc,
            .cbClsExtra = 0,
            .cbWndExtra = 0,
            .hInstance = instance, // Technically we dont NEED to get the instance. WE could easily use GetModuleHandle(NULL) since they both mean the same thing
            .hIcon = nullptr,
            .hCursor = nullptr,
            .hbrBackground = nullptr,
            .lpszMenuName = nullptr,
            .lpszClassName = windowName_,
            .hIconSm = nullptr
        };

        // :: - Look for this function outside the current scope
        if (!::RegisterClassExW(&wc))
        {
            MessageBox(0, L"RegisterClass Failed.", 0, 0);
            return false;
        }

		rect_ =
		{
			.left = 0,
			.top = 0,
			.right = static_cast<LONG>(ctx->Width()),
			.bottom = static_cast<LONG>(ctx->Height())
		};

		std::tie<uint32_t, uint32_t>(clientWidth_, clientHeight_) = utils::ClientRegionDimensions(rect_);

		int monitorWidth = ::GetSystemMetrics(SM_CXSCREEN);
		int monitorHeight = ::GetSystemMetrics(SM_CYSCREEN);

		clientWidth_ = std::clamp<uint32_t>(clientWidth_, 0, monitorWidth);
		clientHeight_ = std::clamp<uint32_t>(clientHeight_, 0, monitorHeight);

		//	put the window in the center of our screen.
		int windowXPos = std::max<int>(0, (monitorWidth - clientWidth_) / 2);
		int windowYPos = std::max<int>(0, (monitorHeight - clientHeight_) / 2);

		// Heres a neat trick, we can pass our context to the CreateWindowEXW function, and in our WindowProc function we can retrieve this baby right back!!
		windowHandle_ = ::CreateWindowExW(0, windowName_, ctx->Name().c_str(), WS_OVERLAPPEDWINDOW, windowXPos, windowYPos,
			clientWidth_, clientHeight_, 0, 0, instance, ctx);


		::GetWindowRect(windowHandle_, &rect_);
		if (!windowHandle_)
		{
			ErrorMessage(L"Failed to create window");
		}


		ctx->Init();


		if (windowHandle_)
		{
			::ShowWindow(windowHandle_, SW_SHOW);
		}
		//d3dApp::ToggleFullScreen();
		// Main game loop
		MSG message{};
		
		while (message.message != WM_QUIT)
		{
			timer_.Tick();

			if (::PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&message);
				::DispatchMessageW(&message);
			}

			ctx->Update();
			ctx->Draw();

		}

		ctx->Destroy();
		::UnregisterClassW(windowName_, instance);

		return static_cast<char>(message.wParam);
	}

	double d3dApp::DeltaTime()
	{
		return timer_.DeltaTime();
	}
	double d3dApp::TotalTime()
	{
		return timer_.TotalTime();
	}
} // namespace