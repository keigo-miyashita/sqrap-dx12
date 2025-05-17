#pragma once

#include <common.hpp>
#include "Buffer.hpp"

class Buffer;
class CommandManager;
class Device;
class Fence;
class Mesh;

class BLAS
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Device* pDevice_ = nullptr;
	Buffer ASBuffer_;
	Buffer scratchBuffer_;
	bool CreateBLAS(const Mesh& mesh, CommandManager& commandManager,Fence& fence, std::wstring name = L"BLAS");

public:

	BLAS();
	~BLAS() = default;
	bool Init(Device* pDevice, const Mesh& mesh, CommandManager& commandManager, Fence& fence, std::wstring name = L"BLAS");

	D3D12_GPU_VIRTUAL_ADDRESS GetASGPUVirtualAddress() const;
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
	bool CreateTLAS(const Mesh& mesh, CommandManager& commandManager, Fence& fence, std::wstring name = L"TLAS");

public:

	TLAS();
	~TLAS() = default;
	bool Init(Device* pDevice, const Mesh& mesh, CommandManager& commandManager, Fence& fence, std::wstring name = L"TLAS");
};