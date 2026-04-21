#include "pch.h"

#include "core/d3dApp.h"
#include "core/DirectXClasses.h"

int WINAPI wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, [[maybe_unused]] _In_ LPWSTR commandLine, [[maybe_unused]] _In_ INT commandSho)
{
	sludge::Config config
	{
		.name = L"Sludge",
		.width = 1920,
		.height = 1080
	};

	sludge::DirectXContext ctx{ config };

	return sludge::d3dApp::Run(&ctx, instance);
}