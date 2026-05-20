#include "Object.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	Object::Object(MeshHandle mesh,
		XMFLOAT4 position,
		XMFLOAT4 quotRotation,
		float scale)
		: mesh_(mesh), position_(position), quotRotation_(quotRotation), scale_(scale)
	{

	}

	XMMATRIX Object::GetModelMat() const
	{
		return XMMatrixTranslation(position_.x, position_.y, position_.z) * XMMatrixRotationQuaternion(XMLoadFloat4(&quotRotation_)) * XMMatrixScaling(scale_, scale_, scale_);
	}

	XMMATRIX Object::GetInvTransMat() const
	{
		return XMMatrixTranspose(XMMatrixInverse(nullptr, GetModelMat()));
	}

	void Object::SetPosition(XMFLOAT4 position)
	{
		position_ = position;
	}

	void Object::SetRotation(XMFLOAT4 quotRotation)
	{
		quotRotation_ = quotRotation;
	}

	void Object::SetScale(float scale)
	{
		scale_ = scale;
	}
}