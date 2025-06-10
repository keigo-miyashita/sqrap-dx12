#pragma once

#include "pch.hpp"

#include "Alias.hpp"

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

class Mesh
{
protected:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	CommandHandle command_;
	std::vector<Vertex> vertices_;
	BufferHandle vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView_;
	std::vector<uint32_t> indices_;
	BufferHandle indexBuffer_;
	D3D12_INDEX_BUFFER_VIEW ibView_;

	static bool LoadModel(std::string modelPath, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
	virtual HRESULT CreateVertexBuffer();
	HRESULT CreateIndexBuffer();

public:
	Mesh(const Device& device, CommandHandle command, std::string modelPath);
	Mesh(const Device& device, CommandHandle command, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	~Mesh() = default;
	BufferHandle GetVertexBuffer() const;
	D3D12_VERTEX_BUFFER_VIEW GetVBView() const;
	const D3D12_VERTEX_BUFFER_VIEW* GetVBViewPtr() const;
	UINT GetVertexCount() const;
	BufferHandle GetIndexBuffer() const;
	D3D12_INDEX_BUFFER_VIEW GetIBView() const;
	const D3D12_INDEX_BUFFER_VIEW* GetIBViewPtr() const;
	UINT GetNumIndices() const;
};

class ASMesh
{
protected:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	CommandHandle command_;
	std::vector<ASVertex> ASVertices_;
	BufferHandle vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView_;
	std::vector<uint32_t> indices_;
	BufferHandle indexBuffer_;
	D3D12_INDEX_BUFFER_VIEW ibView_;

	static bool LoadModelForAS(std::string modelPath, std::vector<ASVertex>& ASVertices, std::vector<uint32_t>& indices);
	HRESULT CreateVertexBuffer();
	HRESULT CreateIndexBuffer();

public:
	ASMesh(const Device& device, CommandHandle command, std::string modelPath);
	ASMesh(const Device& device, CommandHandle command, const std::vector<ASVertex>& ASVertices, const std::vector<uint32_t>& indices);
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