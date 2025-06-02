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

	const Device* pDevice_ = nullptr;
	std::shared_ptr<Command> command_;
	std::wstring name_;
	std::shared_ptr<Buffer> ASBuffer_;
	std::shared_ptr<Buffer> scratchBuffer_;
	bool CreateBLAS(const ASMesh& mesh);

public:

	BLAS(const Device& device, std::shared_ptr<Command> command, const ASMesh& mesh, std::wstring name = L"");
	~BLAS() = default;

	D3D12_GPU_VIRTUAL_ADDRESS GetASAddress();
};

struct TLASDesc
{
	DirectX::XMFLOAT4X4 transform;
	UINT instanceMask = 0x0;
	std::shared_ptr<BLAS> blas;
	D3D12_RAYTRACING_INSTANCE_FLAGS flags;
};

class TLAS
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	std::shared_ptr<Command> command_;
	std::wstring name_;
	std::vector<TLASDesc> tlasDescs_;
	std::shared_ptr<Buffer> instanceDescBuffer_;
	std::shared_ptr<Buffer> ASBuffer_;
	std::shared_ptr<Buffer> scratchBuffer_;
	bool CreateTLAS();

public:

	TLAS(const Device& device, std::shared_ptr<Command> command, const std::vector<TLASDesc>& tlasDescs, std::wstring name = L"");
	~TLAS() = default;
	Buffer GetASBuffer() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetASAddress();
};