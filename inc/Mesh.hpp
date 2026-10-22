#pragma once

#include "pch.hpp"

#include "Alias.hpp"
namespace sqrp
{
	std::wstring StringToWString(const std::string& str);

	class Buffer;
	class Command;
	class Device;

	struct Vertex
	{
		DirectX::XMFLOAT4 position = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT4 normal = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT4 tangent = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT2 uv = { 0.0f, 0.0f };
	};

	struct SubMesh
	{
		UINT indexOffset = 0;
		UINT indexCount = 0;
		int baseVertex = 0;
		int materialIndex = -1;
	};

	struct MeshData
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<SubMesh> subMeshes;
	};

	class Mesh
	{
	protected:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		std::wstring name_ = L"";
		BufferHandle vertexBuffer_;
		D3D12_VERTEX_BUFFER_VIEW vbView_ = {};
		BufferHandle indexBuffer_;
		D3D12_INDEX_BUFFER_VIEW ibView_ = {};
		UINT vertexCount_ = 0;
		UINT indexCount_ = 0;
		std::vector<SubMesh> subMeshes_;

		void CreateVertexBuffer(const CommandHandle& command, const std::vector<Vertex>& vertices);
		void CreateIndexBuffer(const CommandHandle& command, const std::vector<uint32_t>& indices);

	public:
		Mesh(const Device& device, std::wstring name, const CommandHandle& command, std::string modelPath);
		Mesh(const Device& device, std::wstring name, const CommandHandle& command, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;

		BufferHandle GetVertexBuffer() const;
		D3D12_VERTEX_BUFFER_VIEW GetVBView() const;
		const D3D12_VERTEX_BUFFER_VIEW* GetVBViewPtr() const;
		UINT GetVertexCount() const;
		BufferHandle GetIndexBuffer() const;
		D3D12_INDEX_BUFFER_VIEW GetIBView() const;
		const D3D12_INDEX_BUFFER_VIEW* GetIBViewPtr() const;
		UINT GetNumIndices() const;
		const std::vector<SubMesh>& GetSubMeshes() const;
	};
}