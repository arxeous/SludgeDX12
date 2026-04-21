#pragma once

#include "pch.h"

namespace sludge
{
	struct Config
	{
		std::wstring name{};
		uint32_t width{};
		uint32_t height{};
	};

	class IContext
	{
	public:
		IContext(Config& config);
		virtual ~IContext() = default;

		virtual void Init() = 0;
		virtual void Draw() = 0;
		virtual void Update() = 0;
		virtual void Destroy() = 0;
		
		virtual void OnKeyAction(uint8_t keycode, bool isKeyDown) = 0;
		virtual void OnResize() = 0;

		std::wstring Name() const { return name_; };
		uint32_t	 Width() const { return width_; };
		uint32_t	 Height() const { return height_; };


	public:
		// good ol braces just let us default ctor when its empty.
		std::wstring name_{};
		uint32_t	 width_{};
		uint32_t	 height_{};
		float		 aspectRatio_{};
		uint64_t	 frameIndex_{};
	};
}