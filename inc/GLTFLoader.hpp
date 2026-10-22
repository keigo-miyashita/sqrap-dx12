#pragma once

#include "pch.hpp"

#include "Mesh.hpp"

namespace sqrp
{
	struct GLTFNode
	{
		int meshIndex = -1;
		UINT subMeshIndex = 0;
		UINT subMeshCount = 0;
		DirectX::XMMATRIX model = DirectX::XMMatrixIdentity();
		DirectX::XMMATRIX invTransModel = DirectX::XMMatrixIdentity();
	};

	struct GLTFModel
	{
		MeshData mesh;
		std::vector<GLTFNode> nodes;
	};

	GLTFModel LoadGLTF(const std::string& modelPath);
}