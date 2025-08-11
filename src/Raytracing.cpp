#include "Raytracing.hpp"

#include "Device.hpp"
#include "Resource.hpp"
#include "Shader.hpp"
#include "Pipeline.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
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

		if (std::holds_alternative<StateObjectDesc::RayTracingDesc>(stateObject->GetStateObjectDesc().typeDesc_)) {
			StateObjectDesc::RayTracingDesc rtDesc = std::get<StateObjectDesc::RayTracingDesc>(stateObject->GetStateObjectDesc().typeDesc_);

			// All record aligned max record size
			UINT maxRayGenSize = 0;
			for (auto rayGen : rtDesc.rayGens_) {
				UINT shaderSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
				UINT localRootSigSize = 0;
				if (rayGen.localResourceSet_) {
					localRootSigSize = rayGen.localResourceSet_->GetRootSignature()->GetSize();
				}
				UINT rayGenSize = shaderSize + localRootSigSize;
				maxRayGenSize = max(maxRayGenSize, rayGenSize);
			}
			UINT eachRayGenSize = AlignForSBTRecord(maxRayGenSize);
			UINT sumRayGenRegionSize = AlignForSBT(eachRayGenSize * rtDesc.rayGens_.size());

			UINT maxMissSize = 0;
			for (auto miss : rtDesc.misses_) {
				UINT shaderSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
				UINT localRootSigSize = 0;
				if (miss.localResourceSet_) {
					localRootSigSize = miss.localResourceSet_->GetRootSignature()->GetSize();
				}
				UINT missSize = shaderSize + localRootSigSize;
				maxMissSize = max(maxMissSize, missSize);
			}
			UINT eachMissSize = AlignForSBTRecord(maxMissSize);
			UINT sumMissRegionSize = AlignForSBT(eachMissSize * rtDesc.misses_.size());

			UINT maxHitGroupSize = 0;
			for (auto hitGroup : rtDesc.hitGroups_) {
				UINT shaderSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
				UINT localRootSigSize = 0;
				if (hitGroup.localResourceSet_) {
					localRootSigSize = hitGroup.localResourceSet_->GetRootSignature()->GetSize();
				}
				UINT hitGroupSize = shaderSize + localRootSigSize;
				maxHitGroupSize = max(maxHitGroupSize, hitGroupSize);
			}
			UINT eachHitGroupSize = AlignForSBTRecord(maxHitGroupSize);
			UINT sumHitGroupRegionSize = AlignForSBT(eachHitGroupSize * rtDesc.hitGroups_.size());

			UINT allRegionSize = sumRayGenRegionSize + sumMissRegionSize + sumHitGroupRegionSize;

			sbtBuffer_ = pDevice_->CreateBuffer(BufferType::Upload, allRegionSize, 1);

			void* rawPtr = sbtBuffer_->Map();
			// uint8_t is 8 bit = 1 byte data
			// This is suitable for data which is per byte
			if (rawPtr) {
				uint8_t* pSBT = static_cast<uint8_t*>(rawPtr);

				auto pRayGen = pSBT + 0;
				int idRayGen = 0;
				for (auto rayGen : rtDesc.rayGens_) {
					auto pThisRayGen = pRayGen + idRayGen * eachRayGenSize;
					// NOTE : If you use index, you may be ought to use vector not initializer
					void* id = soProp_->GetShaderIdentifier(rayGen.shader_->GetEntryName().c_str());
					pThisRayGen += CopyMem(pThisRayGen, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
					// local root descriptor pointer
					if (!rayGen.localResourceSet_)
						continue;
					const auto resourceSetDescs = rayGen.localResourceSet_->GetResourceSetDescs();
					for (const auto& resourceSetDesc : resourceSetDescs) {
						if (std::holds_alternative<DescriptorManagerHandle>(resourceSetDesc.bindResource)) {
							auto descriptorManagerHandle = std::get<DescriptorManagerHandle>(resourceSetDesc.bindResource);
							auto add = descriptorManagerHandle->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
							pThisRayGen += CopyMem(pThisRayGen, &add, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
						}
						else if (std::holds_alternative<BufferHandle>(resourceSetDesc.bindResource)) {
							auto bufferHandle = std::get<BufferHandle>(resourceSetDesc.bindResource);
							auto add = bufferHandle->GetGPUAddress();
							pThisRayGen += CopyMem(pThisRayGen, &add, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
						}
						else if (std::holds_alternative<ConstantsHandle>(resourceSetDesc.bindResource)) {
							auto constants = std::get<ConstantsHandle>(resourceSetDesc.bindResource);
							// sizeof(float) = sizeof(UINT) = sizeof(int) = 4 Bytes
							pThisRayGen += CopyMem(pThisRayGen, constants.get(), sizeof(float) * constants->GetNumConstants());
						}
					}
					idRayGen++;
				}

				auto pMiss = pSBT + sumRayGenRegionSize;
				int idMiss = 0;
				for (auto miss : rtDesc.misses_) {
					auto pThisMiss = pMiss + idMiss * eachMissSize;

					// NOTE : If you use index, you may be ought to use vector not initializer ?
					void* id = soProp_->GetShaderIdentifier(miss.shader_->GetEntryName().c_str());
					pThisMiss += CopyMem(pThisMiss, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
					// local root descriptor pointer
					if (!miss.localResourceSet_)
						continue;
					const auto resourceSetDescs = miss.localResourceSet_->GetResourceSetDescs();
					for (const auto& resourceSetDesc : resourceSetDescs) {
						if (std::holds_alternative<DescriptorManagerHandle>(resourceSetDesc.bindResource)) {
							auto descriptorManagerHandle = std::get<DescriptorManagerHandle>(resourceSetDesc.bindResource);
							auto add = descriptorManagerHandle->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
							pThisMiss += CopyMem(pThisMiss, &add, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
						}
						else if (std::holds_alternative<BufferHandle>(resourceSetDesc.bindResource)) {
							auto bufferHandle = std::get<BufferHandle>(resourceSetDesc.bindResource);
							auto add = bufferHandle->GetGPUAddress();
							pThisMiss += CopyMem(pThisMiss, &add, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
						}
						else if (std::holds_alternative<ConstantsHandle>(resourceSetDesc.bindResource)) {
							auto constants = std::get<ConstantsHandle>(resourceSetDesc.bindResource);
							// sizeof(float) = sizeof(UINT) = sizeof(int) = 4 Bytes
							pThisMiss += CopyMem(pThisMiss, constants.get(), sizeof(float) * constants->GetNumConstants());
						}
					}
					idMiss++;
				}

				auto pHitGroup = pSBT + sumRayGenRegionSize + sumMissRegionSize;
				int idHitGroup = 0;
				for (auto hitGroup : rtDesc.hitGroups_) {
					auto pThisHitGroup = pHitGroup + idHitGroup * eachHitGroupSize;

					// NOTE : If you use index, you may be ought to use vector not initializer ?
					void* id = soProp_->GetShaderIdentifier(hitGroup.groupName_.c_str());
					pThisHitGroup += CopyMem(pThisHitGroup, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
					// local root descriptor pointer
					if (!hitGroup.localResourceSet_)
						continue;
					const auto resourceSetDescs = hitGroup.localResourceSet_->GetResourceSetDescs();
					for (const auto& resourceSetDesc : resourceSetDescs) {
						if (std::holds_alternative<DescriptorManagerHandle>(resourceSetDesc.bindResource)) {
							auto descriptorManagerHandle = std::get<DescriptorManagerHandle>(resourceSetDesc.bindResource);
							auto add = descriptorManagerHandle->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
							pThisHitGroup += CopyMem(pThisHitGroup, &add, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
						}
						else if (std::holds_alternative<BufferHandle>(resourceSetDesc.bindResource)) {
							auto bufferHandle = std::get<BufferHandle>(resourceSetDesc.bindResource);
							auto add = bufferHandle->GetGPUAddress();
							pThisHitGroup += CopyMem(pThisHitGroup, &add, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
						}
						else if (std::holds_alternative<ConstantsHandle>(resourceSetDesc.bindResource)) {
							auto constants = std::get<ConstantsHandle>(resourceSetDesc.bindResource);
							// sizeof(float) = sizeof(UINT) = sizeof(int) = 4 Bytes
							pThisHitGroup += CopyMem(pThisHitGroup, constants->GetConstants(), sizeof(float) * constants->GetNumConstants());
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
}