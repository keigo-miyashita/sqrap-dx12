#pragma once

#include <common.hpp>

class Camera
{
private:
	DirectX::XMFLOAT4 position_ = { 0.0f, 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 rotation_ = { 0.0f, 0.0f, 0.0f };
	float aspectRatio_;
	float fovYAngle_ = 60.0f;
	float nearZ_ = 0.1f;
	float farZ_ = 100.0f;

public:
	Camera();
	~Camera() = default;
	bool Init(
		float aspectRatio,
		DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f},
		float rotateX = 0.0f,
		float rotateY = 0.0f,
		float fovYAngle = 60.0f,
		float nearZ = 0.1f,
		float farZ = 100.0f
	);

	void Update();
	DirectX::XMFLOAT4 GetPos();
	DirectX::XMFLOAT3 GetFront();
	DirectX::XMFLOAT3 GetUp();
	DirectX::XMFLOAT3 GetRight();
	DirectX::XMMATRIX GetView();
	DirectX::XMMATRIX GetProj();
	DirectX::XMMATRIX GetInvViewProj();
	
};