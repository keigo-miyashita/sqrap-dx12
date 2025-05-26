#pragma once

#include <common.hpp>
#include "Buffer.hpp"

class Buffer;
class Command;
class Device;
class Fence;
class Mesh;
class ASMesh;

class BLAS
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Device* pDevice_ = nullptr;
	Buffer ASBuffer_;
	Buffer scratchBuffer_;
	bool CreateBLAS(const ASMesh& mesh, Command& Command,Fence& fence, std::wstring name = L"BLAS");

public:

	BLAS();
	~BLAS() = default;
	bool Init(Device* pDevice, const ASMesh& mesh, Command& command, Fence& fence, std::wstring name = L"BLAS");

	D3D12_GPU_VIRTUAL_ADDRESS GetASAddress();
};

struct TLASDesc
{
	DirectX::XMFLOAT4X4 transform;
	UINT instanceMask = 0x0;
	BLAS* blas = nullptr;
	D3D12_RAYTRACING_INSTANCE_FLAGS flags;
};

class TLAS
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Device* pDevice_ = nullptr;
	std::vector<TLASDesc> tlasDescs_;
	Buffer instanceDescBuffer_;
	Buffer ASBuffer_;
	Buffer scratchBuffer_;
	bool CreateTLAS(Command& command, Fence& fence, std::wstring name = L"TLAS");

public:

	TLAS();
	~TLAS() = default;
	bool Init(Device* pDevice, Command& command, Fence& fence, std::vector<TLASDesc> tlasDescs, std::wstring name = L"TLAS");
	Buffer GetASBuffer() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetASAddress();
};