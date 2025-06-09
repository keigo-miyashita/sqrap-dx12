#include "Raytracing.hpp"

#include "Device.hpp"
#include "Resource.hpp"
#include "Shader.hpp"
#include "Pipeline.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

UINT CreateSBTBuffer(UINT numRecords, UINT localRootSigSize)
{
	UINT shaderIDSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	UINT recordSize = shaderIDSize + localRootSigSize;
	UINT bufferSize = recordSize * numRecords;

	return bufferSize;
}

UINT RayTracing::AlignForSBTRecord(UINT size)
{
	const UINT alignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
	return (size + (alignment - 1)) & ~(alignment - 1);
}

UINT RayTracing::AlignForSBT(UINT size)
{
	const UINT alignment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	return (size + (alignment - 1)) & ~(alignment - 1);
}

UINT RayTracing::CopyMem(void* dest, const void* data, UINT size)
{
	memcpy(dest, data, size);

	return size;
}

RayTracing::RayTracing(const Device& device, StateObjectHandle stateObject, UINT width, UINT height, UINT depth, std::wstring name)
	: pDevice_(&device), name_(name)
{
	stateObject->GetStateObject()->QueryInterface(IID_PPV_ARGS(soProp_.ReleaseAndGetAddressOf()));

	vector<void*> rayGenIDs;
	vector<void*> missIDs;
	vector<void*> hitGroupIDs;

	if (std::holds_alternative<StateObjectDesc::RayTracingDesc>(stateObject->GetStateObjectDesc().typeDesc)) {
		StateObjectDesc::RayTracingDesc rtDesc = std::get<StateObjectDesc::RayTracingDesc>(stateObject->GetStateObjectDesc().typeDesc);

		// Align size to max record size
		// Allrecord aligned max record size
		UINT maxRayGenSize = 0;
		for (auto rayGen : rtDesc.rayGens) {
			void* id = soProp_->GetShaderIdentifier(rayGen.shader->GetEntryName().c_str());
			UINT shaderSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			UINT localRootSigSize = 0;
			if (rayGen.localResourceSet) {
				localRootSigSize = rayGen.localResourceSet->GetRootSignature()->GetSize();
			}
			UINT rayGenSize = shaderSize + localRootSigSize;
			maxRayGenSize = max(maxRayGenSize, rayGenSize);
		}
		UINT eachRayGenSize = AlignForSBTRecord(maxRayGenSize);
		UINT sumRayGenRegionSize = AlignForSBT(eachRayGenSize * rtDesc.rayGens.size());

		UINT maxMissSize = 0;
		for (auto miss : rtDesc.misses) {
			void* id = soProp_->GetShaderIdentifier(miss.shader->GetEntryName().c_str());
			UINT shaderSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			UINT localRootSigSize = 0;
			if (miss.localResourceSet) {
				localRootSigSize = miss.localResourceSet->GetRootSignature()->GetSize();
			}
			UINT missSize = shaderSize + localRootSigSize;
			maxMissSize = max(maxMissSize, missSize);
		}
		UINT eachMissSize = AlignForSBTRecord(maxMissSize);
		UINT sumMissRegionSize = AlignForSBT(eachMissSize * rtDesc.misses.size());

		UINT maxHitGroupSize = 0;
		for (auto hitGroup : rtDesc.hitGroups) {
			void* id = soProp_->GetShaderIdentifier(hitGroup.groupName.c_str());
			UINT shaderSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			UINT localRootSigSize = 0;
			if (hitGroup.localResourceSet) {
				localRootSigSize = hitGroup.localResourceSet->GetRootSignature()->GetSize();
			}
			UINT hitGroupSize = shaderSize + localRootSigSize;
			maxHitGroupSize = max(maxHitGroupSize, hitGroupSize);
		}
		UINT eachHitGroupSize = AlignForSBTRecord(maxHitGroupSize);
		UINT sumHitGroupRegionSize = AlignForSBT(eachHitGroupSize * rtDesc.hitGroups.size());

		UINT allRegionSize = sumRayGenRegionSize + sumMissRegionSize + sumHitGroupRegionSize;

		sbtBuffer_ = pDevice_->CreateBuffer(
			BufferType::Upload,
			allRegionSize,
			1
		);

		void* rawPtr = sbtBuffer_->Map();
		// uint8_t is 8 bit = 1 byte data
		// This is suitable for data which is per byte
		if (rawPtr) {
			uint8_t* pSBT = static_cast<uint8_t*>(rawPtr);

			auto pRayGen = pSBT + 0;
			int idRayGen = 0;
			for (auto rayGen : rtDesc.rayGens) {
				auto pThisRayGen = pRayGen + idRayGen * eachRayGenSize;
				// NOTE : If you use index, you may be ought to use vector not initializer ?
				void* id = soProp_->GetShaderIdentifier(rayGen.shader->GetEntryName().c_str());
				pThisRayGen += CopyMem(pThisRayGen, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				// local root descriptor pointer
				if (!rayGen.localResourceSet)
					continue;
				const auto bindedResources = rayGen.localResourceSet->GetBindedResources();
				for (const auto& bindedResource : bindedResources) {
					if (std::holds_alternative<D3D12_GPU_DESCRIPTOR_HANDLE>(bindedResource)) {
						auto handle = std::get<D3D12_GPU_DESCRIPTOR_HANDLE>(bindedResource);
						pThisRayGen += CopyMem(pThisRayGen, &handle, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
					}
					else if (std::holds_alternative<D3D12_GPU_VIRTUAL_ADDRESS>(bindedResource)) {
						auto add = std::get<D3D12_GPU_VIRTUAL_ADDRESS>(bindedResource);
						pThisRayGen += CopyMem(pThisRayGen, &add, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
					}
					else if (std::holds_alternative<Constants>(bindedResource)) {
						auto constants = std::get<Constants>(bindedResource);
						// sizeof(float) = sizeof(UINT) = sizeof(int) = 4 Bytes
						pThisRayGen += CopyMem(pThisRayGen, constants.constants, sizeof(float) * constants.numConstants);
					}
				}

				idRayGen++;
			}

			auto pMiss = pSBT + sumRayGenRegionSize;
			int idMiss = 0;
			for (auto miss : rtDesc.misses) {
				auto pThisMiss = pMiss + idMiss * eachMissSize;

				// NOTE : If you use index, you may be ought to use vector not initializer ?
				void* id = soProp_->GetShaderIdentifier(miss.shader->GetEntryName().c_str());
				pThisMiss += CopyMem(pThisMiss, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				// local root descriptor pointer
				if (!miss.localResourceSet)
					continue;
				const auto bindedResources = miss.localResourceSet->GetBindedResources();
				for (const auto& bindedResource : bindedResources) {
					if (std::holds_alternative<D3D12_GPU_DESCRIPTOR_HANDLE>(bindedResource)) {
						auto handle = std::get<D3D12_GPU_DESCRIPTOR_HANDLE>(bindedResource);
						pThisMiss += CopyMem(pThisMiss, &handle, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
					}
					else if (std::holds_alternative<D3D12_GPU_VIRTUAL_ADDRESS>(bindedResource)) {
						auto add = std::get<D3D12_GPU_VIRTUAL_ADDRESS>(bindedResource);
						pThisMiss += CopyMem(pThisMiss, &add, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
					}
					else if (std::holds_alternative<Constants>(bindedResource)) {
						auto constants = std::get<Constants>(bindedResource);
						// sizeof(float) = sizeof(UINT) = sizeof(int) = 4 Bytes
						pThisMiss += CopyMem(pThisMiss, constants.constants, sizeof(float) * constants.numConstants);
					}
				}

				idMiss++;
			}

			auto pHitGroup = pSBT + sumRayGenRegionSize + sumMissRegionSize;
			int idHitGroup = 0;
			for (auto hitGroup : rtDesc.hitGroups) {
				auto pThisHitGroup = pHitGroup + idHitGroup * eachHitGroupSize;

				// NOTE : If you use index, you may be ought to use vector not initializer ?
				void* id = soProp_->GetShaderIdentifier(hitGroup.groupName.c_str());
				pThisHitGroup += CopyMem(pThisHitGroup, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				// local root descriptor pointer
				if (!hitGroup.localResourceSet)
					continue;
				const auto bindedResources = hitGroup.localResourceSet->GetBindedResources();
				for (const auto& bindedResource : bindedResources) {
					if (std::holds_alternative<D3D12_GPU_DESCRIPTOR_HANDLE>(bindedResource)) {
						auto handle = std::get<D3D12_GPU_DESCRIPTOR_HANDLE>(bindedResource);
						pThisHitGroup += CopyMem(pThisHitGroup, &handle, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
					}
					else if (std::holds_alternative<D3D12_GPU_VIRTUAL_ADDRESS>(bindedResource)) {
						auto add = std::get<D3D12_GPU_VIRTUAL_ADDRESS>(bindedResource);
						pThisHitGroup += CopyMem(pThisHitGroup, &add, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
					}
					else if (std::holds_alternative<Constants>(bindedResource)) {
						auto constants = std::get<Constants>(bindedResource);
						// sizeof(float) = sizeof(UINT) = sizeof(int) = 4 Bytes
						pThisHitGroup += CopyMem(pThisHitGroup, constants.constants, sizeof(float) * constants.numConstants);
					}
				}

				idHitGroup++;
			}

			auto startAddress = sbtBuffer_->GetGPUAddress();
			raysDesc_.RayGenerationShaderRecord.StartAddress = startAddress;
			raysDesc_.RayGenerationShaderRecord.SizeInBytes = sumRayGenRegionSize;
			startAddress += sumRayGenRegionSize;

			raysDesc_.MissShaderTable.StartAddress = startAddress;
			raysDesc_.MissShaderTable.SizeInBytes = sumMissRegionSize;
			startAddress += sumMissRegionSize;

			raysDesc_.HitGroupTable.StartAddress = startAddress;
			raysDesc_.HitGroupTable.SizeInBytes = sumHitGroupRegionSize;
			raysDesc_.HitGroupTable.StrideInBytes = eachHitGroupSize;
			startAddress += sumHitGroupRegionSize;

			raysDesc_.Width = width;
			raysDesc_.Height = height;
			raysDesc_.Depth = depth;
		}

	}
}

D3D12_DISPATCH_RAYS_DESC RayTracing::GetDispatchRayDesc() const
{
	return raysDesc_;
}