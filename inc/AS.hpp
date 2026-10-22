#pragma once

#include "pch.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class AS;
	class Buffer;
	class Command;
	class Device;
	class Fence;
	class Mesh;
	class Resource;

	class BLAS
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		std::wstring name_;
		ASHandle ASBuffer_;
		BufferHandle scratchBuffer_;
		void CreateBLAS(const MeshHandle& mesh, const CommandHandle& command);

	public:

		BLAS(const Device& device, std::wstring name, const CommandHandle& command, const MeshHandle& mesh);
		BLAS(const BLAS&) = delete;
		BLAS& operator=(const BLAS&) = delete;

		D3D12_GPU_VIRTUAL_ADDRESS GetASAddress();
	};

	struct TLASDesc
	{
		DirectX::XMMATRIX transform_ = DirectX::XMMatrixIdentity();
		UINT instanceMask_ = 0xff;
		BLASHandle blas_;
		D3D12_RAYTRACING_INSTANCE_FLAGS flags_ = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	};

	class TLAS
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		std::wstring name_;
		std::vector<TLASDesc> tlasDescs_;
		BufferHandle instanceDescBuffer_;
		ASHandle ASBuffer_;
		BufferHandle scratchBuffer_;
		void CreateTLAS(const CommandHandle& command);

	public:

		TLAS(const Device& device, std::wstring name, const CommandHandle& command, const std::vector<TLASDesc>& tlasDescs);
		TLAS(const TLAS&) = delete;
		TLAS& operator=(const TLAS&) = delete;
		ASHandle GetASBuffer() const;
		D3D12_GPU_VIRTUAL_ADDRESS GetASAddress();
	};
}