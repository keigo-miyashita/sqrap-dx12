#pragma once

#include "pch.hpp"

#include "Alias.hpp"

class AS;
class ASMesh;
class Buffer;
class Command;
class Device;
class Fence;
class Mesh;
class Resouce;

class BLAS
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	CommandHandle command_;
	std::wstring name_;
	ASHandle ASBuffer_;
	BufferHandle scratchBuffer_;
	bool CreateBLAS(const ASMesh& mesh);

public:

	BLAS(const Device& device, CommandHandle command, const ASMesh& mesh, std::wstring name = L"");
	~BLAS() = default;

	D3D12_GPU_VIRTUAL_ADDRESS GetASAddress();
};

struct TLASDesc
{
	DirectX::XMMATRIX transform;
	UINT instanceMask = 0x0;
	BLASHandle blas;
	D3D12_RAYTRACING_INSTANCE_FLAGS flags;
};

class TLAS
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	CommandHandle command_;
	std::wstring name_;
	std::vector<TLASDesc> tlasDescs_;
	BufferHandle instanceDescBuffer_;
	ASHandle ASBuffer_;
	BufferHandle scratchBuffer_;
	bool CreateTLAS();

public:

	TLAS(const Device& device, CommandHandle command, const std::vector<TLASDesc>& tlasDescs, std::wstring name = L"");
	~TLAS() = default;
	ASHandle GetASBuffer() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetASAddress();
};