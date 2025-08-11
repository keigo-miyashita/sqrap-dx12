#pragma once

#include "pch.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class Device;
	class RootSignature;

	enum class IndirectType
	{
		Draw, DrawIndexed, Dispatch, VB, IB, Constant, CBV, SRV, UAV, Ray, Mesh
	};

	struct IndirectDesc
	{
		IndirectType type_;
		UINT rootParameterIndex_ = 0;
		UINT numConstant_ = 0;
	};

	class Indirect
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* pDevice_ = nullptr;
		std::wstring name_;
		ComPtr<ID3D12CommandSignature> cmdSig_ = nullptr;
		std::vector<D3D12_INDIRECT_ARGUMENT_DESC> indirectArgDesc_;
		D3D12_COMMAND_SIGNATURE_DESC cmdSigDesc_ = {};
		UINT maxCommandCount_ = 0;
		BufferHandle argumentBuffer_ = nullptr;
		BufferHandle counterBuffer_ = nullptr;

	public:
		Indirect(
			const Device& device,
			std::initializer_list<IndirectDesc> indirectDescs,
			RootSignatureHandle rootSignature,
			UINT byteStride,
			UINT maxCommandCount = 0,
			BufferHandle argumentBuffer = nullptr,
			BufferHandle counterBuffer = nullptr,
			std::wstring name = L""
		);
		~Indirect() = default;
		ComPtr<ID3D12CommandSignature> GetCommandSignature() const;
		UINT GetMaxCommandCount() const;
		BufferHandle GetArgumentBuffer() const;
		BufferHandle GetCounterBuffer() const;
		UINT GetCounterBufferOffset() const;
	};
}