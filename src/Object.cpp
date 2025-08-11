#include "Object.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	Object::Object(MeshHandle mesh, DirectX::XMFLOAT4 position, DirectX::XMFLOAT3 rotation, float scale)
		: mesh_(mesh), position_(position), rotation_(rotation), scale(scale)
	{

	}

	DirectX::XMMATRIX GetModelMat()
	{
		return XMMatrixTranslation(position_.x, position_.y, position_.z) * XMMatrixRotationQuaternion(XMLoadFloat4(&quotRotation_)) * XMMatrixScaling(scale_);
	}

	DirectX::XMMATRIX GetInvTransMat()
	{
		return XMMatrixTranspose(XMMatrixInverse(nullptr, GetModelMat()));
	}

	void SetPosition(DirectX::XMFLOAT4 position)
	{
		position_ = position;
	}

	void SetRotation(DirectX::XMFLOAT3 rotation)
	{
		rotation_ = rotation;
	}

	void SetScale(float scale)
	{
		scale_ = scale;
	}
}