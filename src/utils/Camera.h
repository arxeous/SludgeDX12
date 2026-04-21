#pragma once
#include "pch.h"
#include "utils/utils.h"
using namespace sludge::utils;
namespace sludge
{
	enum class Keys : uint8_t
	{
		W,
		A,
		S,
		D,
		AUp,
		ALeft,
		ADown,
		ARight,
		TotalKeyCount
	};

	static std::map<uint8_t, Keys> INPUT_MAP
	{
		{'W', Keys::W},
		{'A', Keys::A},
		{'S', Keys::S},
		{'D', Keys::D},
		{VK_UP, Keys::AUp},
		{VK_LEFT, Keys::ALeft},
		{VK_DOWN, Keys::ADown},
		{VK_RIGHT, Keys::ARight}
	};


	class Camera
	{
	public:
		void HandleInput(uint8_t keycode, bool isKeyDown);

		void Update(float deltaTime);

		DirectX::XMMATRIX ViewMatrix() const;

		DirectX::XMVECTOR CamPos() const;


	private:
		// Of course every camera needs its defining axes. by default we say the axis from our location
		// to the thing we are looking at is the z axis. But to have some initial value we just make it the "base" world z axis

		DirectX::XMVECTOR xAxis_{ DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f) };
		DirectX::XMVECTOR yAxis_{ DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };
		DirectX::XMVECTOR zAxis_{ DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f) };

		DirectX::XMVECTOR camForward_{ zAxis_ };
		DirectX::XMVECTOR camSide_{ xAxis_ };
		DirectX::XMVECTOR camUp_{ yAxis_ };

		DirectX::XMVECTOR camLookAt_{ DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f) };
		DirectX::XMVECTOR camPos_{ DirectX::XMVectorSet(0.0f, 0.0f, -3.0f, 0.0f) };

		DirectX::XMMATRIX camRotMat_{ DirectX::XMMatrixIdentity() };

		DirectX::XMMATRIX camViewMat_{ DirectX::XMMatrixIdentity() };

		float camSpeed_{ 50.0f };
		float camRotSpeed_{ 0.05f };

		float camYaw_{ 0.0f };
		float camPitch_{ 0.0f };

		std::array<bool, EnumClassValue(Keys::TotalKeyCount)> keyStates_{false};
	};
} // sludge
