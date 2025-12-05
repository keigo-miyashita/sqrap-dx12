#pragma once

#include "pch.hpp"

#include "Alias.hpp"

namespace sqrp
{
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
		std::wstring name_;
		CommandHandle command_;
		ASHandle ASBuffer_;
		BufferHandle scratchBuffer_;
		void CreateBLAS(ASMeshHandle mesh);

	public:

		BLAS(const Device& device, std::wstring name, CommandHandle command, ASMeshHandle mesh);
		~BLAS() = default;

		D3D12_GPU_VIRTUAL_ADDRESS GetASAddress();
	};

	struct TLASDesc
	{
		DirectX::XMMATRIX transform_;
		UINT instanceMask_ = 0x0;
		BLASHandle blas_;
		D3D12_RAYTRACING_INSTANCE_FLAGS flags_;
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
		void CreateTLAS();

	public:

		TLAS(const Device& device, std::wstring name, CommandHandle command, const std::vector<TLASDesc>& tlasDescs);
		~TLAS() = default;
		ASHandle GetASBuffer() const;
		D3D12_GPU_VIRTUAL_ADDRESS GetASAddress();
	};
}