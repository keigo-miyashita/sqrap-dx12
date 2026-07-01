#pragma once

#include "pch.hpp"

#include <map>
#include <utility>

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

	struct ASVertex
	{
		DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	};

	// Common base for vertex/index buffer management
	class MeshBase
	{
	protected:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		std::wstring name_ = L"";
		bool isWritable_ = false;
		CommandHandle command_;
		std::vector<Vertex> vertices_;
		BufferHandle vertexBuffer_;
		D3D12_VERTEX_BUFFER_VIEW vbView_ = {};
		std::vector<uint32_t> indices_;
		BufferHandle indexBuffer_;
		D3D12_INDEX_BUFFER_VIEW ibView_ = {};

		virtual void CreateVertexBuffer();
		void CreateIndexBuffer();

	public:
		MeshBase(const Device& device, CommandHandle command, std::wstring name = L"");
		MeshBase() = default;
		virtual ~MeshBase() = default;

		void UpdateVBView(UINT numVertices);
		void UpdateIBView(UINT numIndices);

		BufferHandle GetVertexBuffer() const;
		D3D12_VERTEX_BUFFER_VIEW GetVBView() const;
		const D3D12_VERTEX_BUFFER_VIEW* GetVBViewPtr() const;
		UINT GetVertexCount() const;
		BufferHandle GetIndexBuffer() const;
		D3D12_INDEX_BUFFER_VIEW GetIBView() const;
		const D3D12_INDEX_BUFFER_VIEW* GetIBViewPtr() const;
		UINT GetNumIndices() const;
	};

	// For simple mesh (flattened glTF / raw vertices / writable)
	class Mesh : public MeshBase
	{
	protected:
		static bool LoadModel(std::string modelPath, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

	public:
		Mesh(const Device& device, CommandHandle command, std::string modelPath);
		Mesh(const Device& device, std::wstring name, CommandHandle command, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
		// For writable mesh
		Mesh(const Device& device, std::wstring name, CommandHandle command, UINT verticesNum, UINT indicesNum);
		~Mesh() = default;
	};

	// For detailed glTF mesh (keeps per-node transforms and per-primitive ranges)
	class GLTFMesh : public MeshBase
	{
	public:
		struct MeshRange
		{
			uint32_t offset = 0;
			uint32_t count = 0;
		};

		struct SubMeshInfo
		{
			DirectX::XMMATRIX model         = DirectX::XMMatrixIdentity();
			DirectX::XMMATRIX invTransModel = DirectX::XMMatrixIdentity();
			int meshIndex = 0;
		};

	protected:
		int meshNum_ = 0;
		std::vector<int> primitiveNumPerMesh_;
		// Per primitive of glTF mesh (= per material). key: (meshIndex, primitiveIndex)
		std::map<std::pair<int, int>, MeshRange> vertexRanges_;
		std::map<std::pair<int, int>, MeshRange> indexRanges_;
		std::map<std::pair<int, int>, int> materialIndices_;
		// SubMeshInfo per node (only nodes that reference a mesh)
		std::vector<SubMeshInfo> subMeshInfos_;

		bool LoadModel(std::string modelPath);

	public:
		GLTFMesh(const Device& device, CommandHandle command, std::string modelPath);
		~GLTFMesh() = default;

		using MeshBase::GetNumIndices;   // keep total-count overload visible

		int GetMeshNum() const;
		int GetPrimitiveNumPerMesh(int meshIndex) const;
		MeshRange GetVertexRange(int meshIndex, int primitiveIndex) const;
		MeshRange GetIndexRange(int meshIndex, int primitiveIndex) const;
		int GetMaterialIndex(int meshIndex, int primitiveIndex) const;
		const std::vector<SubMeshInfo>& GetSubMeshInfos() const;
		int GetNumIndices(int meshIndex, int primitiveIndex) const;
		DirectX::XMMATRIX GetGltfModelMat() const;
	};

	class ASMesh
	{
	protected:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		std::wstring name_ = L"";
		CommandHandle command_;
		std::vector<ASVertex> ASVertices_;
		BufferHandle vertexBuffer_;
		D3D12_VERTEX_BUFFER_VIEW vbView_;
		std::vector<uint32_t> indices_;
		BufferHandle indexBuffer_;
		D3D12_INDEX_BUFFER_VIEW ibView_;

		static bool LoadModelForAS(std::string modelPath, std::vector<ASVertex>& ASVertices, std::vector<uint32_t>& indices);
		void CreateVertexBuffer();
		void CreateIndexBuffer();

	public:
		ASMesh(const Device& device, CommandHandle command, std::string modelPath);
		ASMesh(const Device& device, std::wstring name, CommandHandle command, const std::vector<ASVertex>& ASVertices, const std::vector<uint32_t>& indices);
		~ASMesh() = default;
		BufferHandle GetVertexBuffer() const;
		D3D12_VERTEX_BUFFER_VIEW GetVBView() const;
		const D3D12_VERTEX_BUFFER_VIEW* GetVBViewPtr() const;
		UINT GetVertexCount() const;
		BufferHandle GetIndexBuffer() const;
		D3D12_INDEX_BUFFER_VIEW GetIBView() const;
		const D3D12_INDEX_BUFFER_VIEW* GetIBViewPtr() const;
		UINT GetNumIndices() const;
	};
}