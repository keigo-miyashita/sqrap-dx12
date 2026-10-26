#pragma once

#include "pch.hpp"

#include "RootSignature.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class Buffer;
	class Device;
	class Shader;
	class StateObject;

	class Raytracing
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		std::wstring name_;
		ComPtr<ID3D12StateObjectProperties> soProp_ = nullptr;
		BufferHandle sbtBuffer_;

		UINT rayGenNum_ = 0;
		UINT64 eachRayGenSize_ = 0;
		UINT64 sumRayGenRegionSize_ = 0;
		UINT64 eachMissSize_ = 0;
		UINT64 sumMissRegionSize_ = 0;
		UINT64 eachHitGroupSize_ = 0;
		UINT64 sumHitGroupRegionSize_ = 0;

	public:
		static UINT64 AlignForSBTRecord(UINT64 size);
		static UINT64 AlignForSBT(UINT64 size);
		static UINT64 CopyMem(void* dest, const void* data, UINT64 size);
		D3D12_DISPATCH_RAYS_DESC MakeDispatchRaysDesc(UINT rayGenIndex, UINT width, UINT height, UINT depth) const;
		Raytracing(const Device& device, std::wstring name, const StateObjectHandle& stateObject, 
				   std::vector<std::vector<LocalRootArguments>> rayGenLocalRootArguments,
				   std::vector<std::vector<LocalRootArguments>> missLocalRootArguments,
				   std::vector<std::vector<LocalRootArguments>> hitGroupLocalRootArguments);
		Raytracing(const Raytracing&) = delete;
		Raytracing& operator=(const Raytracing&) = delete;
	};
}