#pragma once

#include <common.hpp>

struct Vertex
{
	DirectX::XMFLOAT4 position = {0.0f, 0.0f, 0.0f, 1.0f};
	DirectX::XMFLOAT4 normal = { 0.0f, 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT4 tangent = { 0.0f, 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT2 uv = { 0.0f, 0.0f };
};

class Device;

class Mesh
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Device* pDevice_ = nullptr;
	std::vector<Vertex> vertices_;
	ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView_;
	std::vector<uint32_t> indices_;
	ComPtr<ID3D12Resource> indexBuffer_;
	D3D12_INDEX_BUFFER_VIEW ibView_;

	bool LoadModel(std::string modelPath);
	HRESULT CreateVertexBuffer();
	HRESULT CreateIndexBuffer();

public:
	Mesh();
	~Mesh() = default;
	bool Init(Device* pDevice, std::string modelPath);
	bool Init(Device* pDevice, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	ComPtr<ID3D12Resource> GetVertexBuffer() const;
	D3D12_VERTEX_BUFFER_VIEW GetVBView() const;
	const D3D12_VERTEX_BUFFER_VIEW* GetVBViewPtr() const;
	UINT GetVertexCount() const;
	ComPtr<ID3D12Resource> GetIndexBuffer() const;
	D3D12_INDEX_BUFFER_VIEW GetIBView() const;
	const D3D12_INDEX_BUFFER_VIEW* GetIBViewPtr() const;
	UINT GetNumIndices() const;
};