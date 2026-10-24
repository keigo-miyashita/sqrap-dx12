#pragma once

#include "pch.hpp"

#include "DescriptorLayout.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class Buffer;
	class Device;
	class Resource;
	class Texture;

	struct DescriptorManagerDesc
	{
		ResourceHandle resource_;
		UINT regIndex_ = 0;
		bool isCounter_ = false;
		D3D12_SAMPLER_DESC samplerDesc_ = {};
		UINT mipSlice_ = 0;        // UAV用：書き込み対象のmip
	};

	class DescriptorManager
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		DescriptorLayoutType descLayoutType_ = DescriptorLayoutType::Resource;
		D3D12_GPU_DESCRIPTOR_HANDLE descHeapGPUHandle_ = {};

	public:
		DescriptorManager(const Device& device, std::initializer_list<DescriptorManagerDesc> descManagerDesc, DescriptorLayout descLayout);
		DescriptorManager(const DescriptorManager&) = delete;
		DescriptorManager& operator=(const DescriptorManager&) = delete;
		DescriptorLayoutType GetDescriptorLayoutType() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetDescHeapGPUHandle() const;
	};
}
