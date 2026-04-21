#include "Camera.h"
#include "pch.h"

// DirectX has the overloads for vector * so we use the namespace here in this translation unit.
using namespace DirectX;
using namespace sludge::utils;
namespace sludge
{
	void Camera::HandleInput(uint8_t keycode, bool isKeyDown)
	{
		if (INPUT_MAP.find(keycode) != std::end(INPUT_MAP))
		{
			auto idx = EnumClassValue(INPUT_MAP[keycode]);
			keyStates_[idx] = isKeyDown;
		}
	}


	void Camera::Update(float delta)
	{
		float movementSpeed = delta * camSpeed_ / 10.0f;

		if (keyStates_[EnumClassValue(Keys::W)])
		{
			camPos_ += camForward_ * movementSpeed;
		}
		else if (keyStates_[EnumClassValue(Keys::S)])
		{
			camPos_ -= camForward_ * movementSpeed;
		}

		if (keyStates_[EnumClassValue(Keys::A)])
		{
			camPos_ -= camSide_ * movementSpeed;
		}
		else if (keyStates_[EnumClassValue(Keys::D)])
		{
			camPos_ += camSide_ * movementSpeed;
		}

		if (keyStates_[EnumClassValue(Keys::AUp)])
		{
			camPitch_ -= camRotSpeed_;
		}
		else if (keyStates_[EnumClassValue(Keys::ADown)])
		{
			camPitch_ += camRotSpeed_;
		}

		if (keyStates_[EnumClassValue(Keys::ALeft)])
		{
			camYaw_ -= camRotSpeed_;
		}
		else if (keyStates_[EnumClassValue(Keys::ARight)])
		{
			camYaw_ += camRotSpeed_;
		}


		// Regular old camera coordinate space creation. The pitch and yaw sub in for a the exact "point" we are looking at
		// By default we just assume the camera is looking through the z axis so we transform it into the coordinate space
		// defined by the pitch and yaw we have. The rest is easy enough to follow.
		camRotMat_ = DirectX::XMMatrixRotationRollPitchYaw(camPitch_, camYaw_, 0.0f);
		camLookAt_ = DirectX::XMVector3TransformCoord(zAxis_, camRotMat_);
		camLookAt_ = DirectX::XMVector3Normalize(camLookAt_);

		camSide_ = DirectX::XMVector3TransformCoord(xAxis_, camRotMat_);
		camForward_ = DirectX::XMVector3TransformCoord(zAxis_, camRotMat_);
		camUp_ = DirectX::XMVector3Cross(camForward_, camSide_);

		camLookAt_ = camPos_ + camLookAt_;

		camViewMat_ = DirectX::XMMatrixLookAtLH(camPos_, camLookAt_, camUp_);
	}

	DirectX::XMMATRIX Camera::ViewMatrix() const
	{
		return camViewMat_;
	}
	DirectX::XMVECTOR Camera::CamPos() const
	{
		return camPos_;
	}
}