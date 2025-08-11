#include "Object.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	Object::Object(MeshHandle mesh,
		DirectX::XMFLOAT4 position,
		DirectX::XMFLOAT4 quotRotation,
		float scale)
		: mesh_(mesh), position_(position), quotRotation_(quotRotation), scale_(scale)
	{

	}

	DirectX::XMMATRIX Object::GetModelMat()
	{
		return XMMatrixTranslation(position_.x, position_.y, position_.z) * XMMatrixRotationQuaternion(XMLoadFloat4(&quotRotation_)) * XMMatrixScaling(scale_, scale_, scale_);
	}

	DirectX::XMMATRIX Object::GetInvTransMat()
	{
		return XMMatrixTranspose(XMMatrixInverse(nullptr, GetModelMat()));
	}

	void Object::SetPosition(DirectX::XMFLOAT4 position)
	{
		position_ = position;
	}

	void Object::SetRotation(DirectX::XMFLOAT4 quotRotation)
	{
		quotRotation_ = quotRotation;
	}

	void Object::SetScale(float scale)
	{
		scale_ = scale;
	}
}