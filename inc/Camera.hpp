#pragma once

#include "pch.hpp"

namespace sqrp
{
	struct CameraMatrix
	{
		DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
		DirectX::XMMATRIX proj = DirectX::XMMatrixIdentity();
	};

	enum class CameraMode
	{
		FreeMove,
		Orbital,
	};

	class Camera
	{
	private:
		CameraMode mode_ = CameraMode::FreeMove;

		// FreeMove
		DirectX::XMFLOAT4 position_ = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 rotation_ = { 0.0f, 0.0f, 0.0f };
		float moveScale_ = 0.05f;

		// Orbital
		DirectX::XMFLOAT3 target_ = { 0.0f, 0.0f, 0.0f };
		float radius_ = 5.0f;
		float azimuth_ = 0.0f;
		float elevation_ = 0.0f;
		float minRadius_ = 0.1f;
		float maxRadius_ = 1000.0f;
		float zoomScale_ = 0.001f;
		float panScale_ = 0.002f;

		// Common
		float aspectRatio_ = 1.0f;
		float fovYAngle_ = 60.0f;
		float nearZ_ = 0.1f;
		float farZ_ = 100.0f;
		float rotateScale_ = 0.05f;

		DirectX::XMFLOAT3 ComputeOrbitalPosition() const;
		void UpdateFreeMove();
		void UpdateOrbital();

	public:
		Camera() = default;

		bool Init(
			float aspectRatio,
			DirectX::XMFLOAT3 position,
			float rotateX = 0.0f,
			float rotateY = 0.0f,
			float fovYAngle = 60.0f,
			float nearZ = 0.1f,
			float farZ = 100.0f
		);

		void SetMode(CameraMode mode);
		CameraMode GetMode() const;

		void Update();
		DirectX::XMFLOAT4 GetPos() const;
		DirectX::XMFLOAT3 GetRotation() const;
		DirectX::XMFLOAT3 GetFront() const;
		DirectX::XMFLOAT3 GetUp() const;
		DirectX::XMFLOAT3 GetRight() const;
		DirectX::XMFLOAT3 GetTarget() const;
		DirectX::XMMATRIX GetView() const;
		DirectX::XMMATRIX GetProj() const;
		DirectX::XMMATRIX GetInvViewProj() const;
		DirectX::XMMATRIX GetInvView() const;

		float GetMoveScale() const;
		float GetRotateScale() const;
		float GetZoomScale() const;
		float GetPanScale() const;

		void SetMoveScale(float scale);
		void SetRotateScale(float scale);
		void SetZoomScale(float scale);
		void SetPanScale(float scale);
		void SetPosition(DirectX::XMFLOAT3 position);
		void SetRotation(DirectX::XMFLOAT3 rotation);
		void SetTarget(DirectX::XMFLOAT3 target);
		void SetRadius(float radius);
		void SetAspectRatio(float aspectRatio);
	};
}