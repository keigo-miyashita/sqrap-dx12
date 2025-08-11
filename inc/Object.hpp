#pragma once

#include "pch.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class Mesh;

	struct TransformMatrix
	{
		DirectX::XMMATRIX model= DirectX::XMMatrixIdentity();
		DirectX::XMMATRIX invTransModel = DirectX::XMMatrixIdentity();
	};

	class Object
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

	private:
		MeshHandle mesh_;
		DirectX::XMFLOAT4 position_;
		DirectX::XMFLOAT4 quotRotation_;
		float scale_;

	public:
		Object(
			MeshHandle mesh,
			DirectX::XMFLOAT4 position = {0.0f, 0.0f, 0.0f, 1.0f},
			DirectX::XMFLOAT4 quotRotation = {0.0f, 0.0f, 0.0f, 0.0f},
			float scale = 1.0f
		);
		~Object() = default;

		DirectX::XMMATRIX GetModelMat();
		DirectX::XMMATRIX GetInvTransMat();
		
		void SetPosition(DirectX::XMFLOAT4 position);
		void SetRotation(DirectX::XMFLOAT4 rotation);
		void SetScale(float scale);
	};
}