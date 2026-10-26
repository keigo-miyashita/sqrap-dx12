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
		bool isCounter_ = false;
		UINT mipSlice_ = 0;        // UAV用：書き込み対象のmip
	};

	class DescriptorManager
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		DescriptorLayoutType descLayoutType_ = DescriptorLayoutType::Resource;
		D3D12_GPU_DESCRIPTOR_HANDLE descHeapGPUHandle_ = {};

		// Allocates the range for descLayout in the heap of its type and returns the first index
		UINT AllocateDescriptors(const Device& device, const DescriptorLayout& descLayout, size_t numDescs);

	public:
		// For a Resource layout
		DescriptorManager(const Device& device, std::initializer_list<DescriptorManagerDesc> descManagerDesc, DescriptorLayout descLayout);
		// For a Sampler layout
		DescriptorManager(const Device& device, std::initializer_list<D3D12_SAMPLER_DESC> samplerDescs, DescriptorLayout descLayout);
		DescriptorManager(const DescriptorManager&) = delete;
		DescriptorManager& operator=(const DescriptorManager&) = delete;
		DescriptorLayoutType GetDescriptorLayoutType() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetDescHeapGPUHandle() const;
	};
}
