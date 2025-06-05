#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

Camera::Camera()
{

}

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

	return true;
}

void Camera::Update()
{
	XMFLOAT3 front = GetFront();
	XMFLOAT3 right = GetRight();
	XMFLOAT3 up = GetUp();
	float moveScale = 0.1f;
	if (Input::IsPushKey('W')) {
		position_.x += front.x * moveScale;
		position_.y += front.y * moveScale;
		position_.z += front.z * moveScale;
	}
	else if (Input::IsPushKey('S')) {
		position_.x -= front.x * moveScale;
		position_.y -= front.y * moveScale;
		position_.z -= front.z * moveScale;
	}
	else if (Input::IsPushKey('D')) {
		position_.x += right.x * moveScale;
		position_.y += right.y * moveScale;
		position_.z += right.z * moveScale;
	}
	else if (Input::IsPushKey('A')) {
		position_.x -= right.x * moveScale;
		position_.y -= right.y * moveScale;
		position_.z -= right.z * moveScale;
	}
	else if (Input::IsPushKey(VK_SPACE)) {
		position_.x += up.x * moveScale;
		position_.y += up.y * moveScale;
		position_.z += up.z * moveScale;
	}
	else if (Input::IsPushKey(VK_CONTROL)) {
		position_.x -= up.x * moveScale;
		position_.y -= up.y * moveScale;
		position_.z -= up.z * moveScale;
	}

	if (Input::IsPushedLButton()) {
		auto pushedMousePos = Input::GetPushedPos();
		auto currentMousePos = Input::GetPos();

		rotation_.x += (currentMousePos.y - pushedMousePos.y) * moveScale;
		rotation_.y += (currentMousePos.x - pushedMousePos.x) * moveScale;
	}
}

DirectX::XMFLOAT4 Camera::GetPos()
{
	return position_;
}

DirectX::XMFLOAT3 Camera::GetFront()
{
	XMVECTOR defaultVec = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMMATRIX rotationX = XMMatrixRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), XMConvertToRadians(rotation_.x));
	XMMATRIX rotationY = XMMatrixRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XMConvertToRadians(rotation_.y));
	
	XMFLOAT3 frontDir;
	XMStoreFloat3(&frontDir, XMVector3Normalize((XMVector3Transform(defaultVec, rotationX * rotationY))));

	return frontDir;
}

DirectX::XMFLOAT3 Camera::GetUp()
{
	XMFLOAT3 frontDir = GetFront();
	XMFLOAT3 rightDir = GetRight();

	XMFLOAT3 upDir;
	XMStoreFloat3(&upDir, XMVector3Normalize((XMVector3Cross(XMLoadFloat3(&frontDir), XMLoadFloat3(&rightDir)))));
	return upDir;
}

DirectX::XMFLOAT3 Camera::GetRight()
{
	XMFLOAT3 frontDir = GetFront();
	XMFLOAT3 yPlusDir = XMFLOAT3(0.0f, 1.0f, 0.0f);

	XMFLOAT3 rightDir;
	XMStoreFloat3(&rightDir, XMVector3Normalize((XMVector3Cross(XMLoadFloat3(&yPlusDir), XMLoadFloat3(&frontDir)))));
	return rightDir;
}

DirectX::XMMATRIX Camera::GetView()
{
	XMFLOAT4 targetPos;
	XMFLOAT4 cameraPos = position_;
	XMFLOAT3 frontDir = GetFront();
	XMFLOAT3 upDir = GetUp();
	XMStoreFloat4(&targetPos, XMLoadFloat4(&cameraPos) + XMLoadFloat3(&frontDir));
	return XMMatrixLookAtLH(XMLoadFloat4(&cameraPos), XMLoadFloat4(&targetPos), XMLoadFloat3(&upDir));
}

DirectX::XMMATRIX Camera::GetProj()
{
	return XMMatrixPerspectiveFovLH(XMConvertToRadians(fovYAngle_), aspectRatio_, nearZ_, farZ_);
}

DirectX::XMMATRIX Camera::GetInvViewProj()
{
	return XMMatrixInverse(nullptr, XMMatrixMultiply(GetView(), GetProj()));
}

DirectX::XMMATRIX Camera::GetInvView()
{
	return XMMatrixInverse(nullptr, GetView());
}