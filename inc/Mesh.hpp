#pragma once

#include <common.hpp>

struct Vertex
{
	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT4 normal;
	DirectX::XMFLOAT4 tangent;
	DirectX::XMFLOAT2 uv;
};

class Device;

class Mesh
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	std::vector<Vertex> vertices_;
	ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView_;
	std::vector<uint32_t> indices_;
	ComPtr<ID3D12Resource> indexBuffer_;
	D3D12_INDEX_BUFFER_VIEW ibView_;

	bool LoadModel(std::string modelPath);
	HRESULT CreateVertexBuffer(const Device& device);
	HRESULT CreateIndexBuffer(const Device& device);

public:
	Mesh();
	~Mesh() = default;
	bool Init(const Device& device, std::string modelPath);
	ComPtr<ID3D12Resource> GetVertexBuffer() const;
	D3D12_VERTEX_BUFFER_VIEW GetVBView() const;
	const D3D12_VERTEX_BUFFER_VIEW* GetVBViewPtr() const;
	ComPtr<ID3D12Resource> GetIndexBuffer() const;
	D3D12_INDEX_BUFFER_VIEW GetIBView() const;
	const D3D12_INDEX_BUFFER_VIEW* GetIBViewPtr() const;
	UINT GetNumIndices() const;
};