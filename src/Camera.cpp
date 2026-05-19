#include "Camera.hpp"

#include "Application.hpp"

using namespace std;
using namespace DirectX;

namespace sqrp
{
	bool Camera::Init(
		float aspectRatio,
		DirectX::XMFLOAT3 position,
		float rotateX,
		float rotateY,
		float fovYAngle,
		float nearZ,
		float farZ
	)
	{
		aspectRatio_ = aspectRatio;
		position_ = { position.x, position.y, position.z, 1.0f };
		rotation_ = XMFLOAT3(rotateX, rotateY, 0.0f);
		fovYAngle_ = fovYAngle;
		nearZ_ = nearZ;
		farZ_ = farZ;
		mode_ = CameraMode::FreeMove;

		return true;
	}

	// azimuth_ / elevation_ は rotation_.y / rotation_.x と同じ角度規則を使う
	// position = target - radius * front(azimuth, elevation)
	XMFLOAT3 Camera::ComputeOrbitalPosition() const
	{
		float azRad = XMConvertToRadians(azimuth_);
		float elRad = XMConvertToRadians(elevation_);
		float cosEl = cosf(elRad);
		XMFLOAT3 front(sinf(azRad) * cosEl, -sinf(elRad), cosf(azRad) * cosEl);
		return XMFLOAT3(
			target_.x - radius_ * front.x,
			target_.y - radius_ * front.y,
			target_.z - radius_ * front.z
		);
	}

	void Camera::SetMode(CameraMode mode)
	{
		if (mode == mode_) return;

		if (mode == CameraMode::Orbital) {
			// FreeMove → Orbital: 現在の視線方向・位置を引き継ぐ
			azimuth_ = rotation_.y;
			elevation_ = std::clamp(rotation_.x, -89.0f, 89.0f);
			XMFLOAT3 front = GetFront();
			target_.x = position_.x + front.x * radius_;
			target_.y = position_.y + front.y * radius_;
			target_.z = position_.z + front.z * radius_;
		}
		else {
			// Orbital → FreeMove: オービタル状態から位置・回転を引き継ぐ
			XMFLOAT3 pos = ComputeOrbitalPosition();
			position_ = XMFLOAT4(pos.x, pos.y, pos.z, 1.0f);
			rotation_ = XMFLOAT3(elevation_, azimuth_, 0.0f);
		}

		mode_ = mode;
	}

	CameraMode Camera::GetMode() const
	{
		return mode_;
	}

	void Camera::UpdateFreeMove()
	{
		XMFLOAT3 front = GetFront();
		XMFLOAT3 right = GetRight();
		XMFLOAT3 up = GetUp();
		if (Input::IsPushKey('W')) {
			position_.x += front.x * moveScale_;
			position_.y += front.y * moveScale_;
			position_.z += front.z * moveScale_;
		}
		if (Input::IsPushKey('S')) {
			position_.x -= front.x * moveScale_;
			position_.y -= front.y * moveScale_;
			position_.z -= front.z * moveScale_;
		}
		if (Input::IsPushKey('D')) {
			position_.x += right.x * moveScale_;
			position_.y += right.y * moveScale_;
			position_.z += right.z * moveScale_;
		}
		if (Input::IsPushKey('A')) {
			position_.x -= right.x * moveScale_;
			position_.y -= right.y * moveScale_;
			position_.z -= right.z * moveScale_;
		}
		if (Input::IsPushKey(VK_SPACE)) {
			position_.x += up.x * moveScale_;
			position_.y += up.y * moveScale_;
			position_.z += up.z * moveScale_;
		}
		if (Input::IsPushKey(VK_CONTROL)) {
			position_.x -= up.x * moveScale_;
			position_.y -= up.y * moveScale_;
			position_.z -= up.z * moveScale_;
		}

		if (Input::IsPushedLButton()) {
			rotation_.x += Input::GetDeltaPos().y * rotateScale_;
			rotation_.y += Input::GetDeltaPos().x * rotateScale_;
		}
	}

	void Camera::UpdateOrbital()
	{
		// 左ドラッグ: オービット（水平・垂直回転）
		if (Input::IsPushedLButton()) {
			azimuth_ += Input::GetDeltaPos().x * rotateScale_;
			elevation_ += Input::GetDeltaPos().y * rotateScale_;
			elevation_ = std::clamp(elevation_, -89.0f, 89.0f);
		}

		// ホイール: ズーム（radius を比率で変化させて距離感を一定に保つ）
		int wheel = Input::GetWheel();
		if (wheel != 0) {
			radius_ *= (1.0f - static_cast<float>(wheel) * zoomScale_);
			radius_ = std::clamp(radius_, minRadius_, maxRadius_);
		}

		// 右ドラッグ: パン（ターゲット点をカメラ平面内で移動）
		if (Input::IsPushedRButton()) {
			float azRad = XMConvertToRadians(azimuth_);
			float elRad = XMConvertToRadians(elevation_);
			XMFLOAT3 front(sinf(azRad) * cosf(elRad), -sinf(elRad), cosf(azRad) * cosf(elRad));
			XMFLOAT3 worldUp(0.0f, 1.0f, 0.0f);
			XMFLOAT3 right, camUp;
			XMStoreFloat3(&right, XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&worldUp), XMLoadFloat3(&front))));
			XMStoreFloat3(&camUp, XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&front), XMLoadFloat3(&right))));

			// マウス右移動でシーンが右にスクロールするよう target を左へ移動
			float scale = panScale_ * radius_;
			auto delta = Input::GetDeltaPos();
			target_.x += (-right.x * delta.x + camUp.x * delta.y) * scale;
			target_.y += (-right.y * delta.x + camUp.y * delta.y) * scale;
			target_.z += (-right.z * delta.x + camUp.z * delta.y) * scale;
		}
	}

	void Camera::Update()
	{
		if (mode_ == CameraMode::FreeMove)
			UpdateFreeMove();
		else
			UpdateOrbital();
	}

	XMFLOAT4 Camera::GetPos()
	{
		if (mode_ == CameraMode::Orbital) {
			XMFLOAT3 pos = ComputeOrbitalPosition();
			return XMFLOAT4(pos.x, pos.y, pos.z, 1.0f);
		}
		return position_;
	}

	XMFLOAT3 Camera::GetRotation()
	{
		return rotation_;
	}

	XMFLOAT3 Camera::GetFront()
	{
		XMVECTOR defaultVec = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		XMMATRIX rotationX = XMMatrixRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), XMConvertToRadians(rotation_.x));
		XMMATRIX rotationY = XMMatrixRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XMConvertToRadians(rotation_.y));

		XMFLOAT3 frontDir;
		XMStoreFloat3(&frontDir, XMVector3Normalize((XMVector3Transform(defaultVec, rotationX * rotationY))));

		return frontDir;
	}

	XMFLOAT3 Camera::GetUp()
	{
		XMFLOAT3 frontDir = GetFront();
		XMFLOAT3 rightDir = GetRight();

		XMFLOAT3 upDir;
		XMStoreFloat3(&upDir, XMVector3Normalize((XMVector3Cross(XMLoadFloat3(&frontDir), XMLoadFloat3(&rightDir)))));
		return upDir;
	}

	XMFLOAT3 Camera::GetRight()
	{
		XMFLOAT3 frontDir = GetFront();
		XMFLOAT3 yPlusDir = XMFLOAT3(0.0f, 1.0f, 0.0f);

		XMFLOAT3 rightDir;
		XMStoreFloat3(&rightDir, XMVector3Normalize((XMVector3Cross(XMLoadFloat3(&yPlusDir), XMLoadFloat3(&frontDir)))));
		return rightDir;
	}

	XMFLOAT3 Camera::GetTarget()
	{
		return target_;
	}

	XMMATRIX Camera::GetView()
	{
		if (mode_ == CameraMode::Orbital) {
			XMFLOAT3 pos = ComputeOrbitalPosition();
			XMFLOAT3 worldUp(0.0f, 1.0f, 0.0f);
			return XMMatrixLookAtLH(XMLoadFloat3(&pos), XMLoadFloat3(&target_), XMLoadFloat3(&worldUp));
		}

		XMFLOAT4 targetPos;
		XMFLOAT4 cameraPos = position_;
		XMFLOAT3 frontDir = GetFront();
		XMFLOAT3 upDir = GetUp();
		XMStoreFloat4(&targetPos, XMLoadFloat4(&cameraPos) + XMLoadFloat3(&frontDir));
		return XMMatrixLookAtLH(XMLoadFloat4(&cameraPos), XMLoadFloat4(&targetPos), XMLoadFloat3(&upDir));
	}

	XMMATRIX Camera::GetProj()
	{
		return XMMatrixPerspectiveFovLH(XMConvertToRadians(fovYAngle_), aspectRatio_, nearZ_, farZ_);
	}

	XMMATRIX Camera::GetInvViewProj()
	{
		return XMMatrixInverse(nullptr, XMMatrixMultiply(GetView(), GetProj()));
	}

	XMMATRIX Camera::GetInvView()
	{
		return XMMatrixInverse(nullptr, GetView());
	}

	float Camera::GetMoveScale()
	{
		return moveScale_;
	}

	float Camera::GetRotateScale()
	{
		return rotateScale_;
	}

	float Camera::GetZoomScale()
	{
		return zoomScale_;
	}

	float Camera::GetPanScale()
	{
		return panScale_;
	}

	float* Camera::GetMoveScalePtr()
	{
		return &moveScale_;
	}

	float* Camera::GetRotateScalePtr()
	{
		return &rotateScale_;
	}

	float* Camera::GetZoomScalePtr()
	{
		return &zoomScale_;
	}

	float* Camera::GetPanScalePtr()
	{
		return &panScale_;
	}

	void Camera::SetMoveScale(float scale)
	{
		moveScale_ = scale;
	}

	void Camera::SetRotateScale(float scale)
	{
		rotateScale_ = scale;
	}

	void Camera::SetPosition(DirectX::XMFLOAT3 position)
	{
		position_ = DirectX::XMFLOAT4(position.x, position.y, position.z, 1.0f);
	}

	void Camera::SetRotation(DirectX::XMFLOAT3 rotation)
	{
		rotation_ = rotation;
	}

	void Camera::SetTarget(DirectX::XMFLOAT3 target)
	{
		target_ = target;
	}

	void Camera::SetRadius(float radius)
	{
		radius_ = std::clamp(radius, minRadius_, maxRadius_);
	}
}
