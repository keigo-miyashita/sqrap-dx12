#pragma once

#include "pch.hpp"

#include "Alias.hpp"

class Mesh;

namespace sqrp
{
	struct TransformMatrix
	{
		DirectX::XMMATRIX modelMatrix;
		DirectX::XMMATRIX invTransMatrix;
	};

	class Object
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

	private:
		MeshHandle mesh_;
		DirectX::XMFLOAT4 position_;
		DirectX::XMFLOAT3 rotation_;
		float scale_;

	public:
		Object(
			MeshHandle mesh,
			DirectX::XMFLOAT4 position = {0.0f, 0.0f, 0.0f, 1.0f},
			DirectX::XMFLOAT4 quotRotation_ = {0.0f, 0.0f, 0.0f, 0.0f},
			float scale = 1.0f;
		);
		~Object() = default;

		DirectX::XMMATRIX GetModelMat();
		DirectX::XMMATRIX GetInvTransMat();
		
		void SetPosition(DirectX::XMFLOAT4 position);
		void SetRotation(DirectX::XMFLOAT3 rotation);
		void SetScale(float scale);
	};
}