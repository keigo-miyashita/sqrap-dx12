#include "Raytracing.hpp"

#include "Command.hpp"
#include "DescriptorManager.hpp"
#include "Device.hpp"
#include "Resource.hpp"
#include "RootSignature.hpp"
#include "Shader.hpp"
#include "Pipeline.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	UINT64 Raytracing::AlignForSBTRecord(UINT64 size)
	{
		const UINT64 alignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
		return (size + (alignment - 1)) & ~(alignment - 1);
	}

	UINT64 Raytracing::AlignForSBT(UINT64 size)
	{
		const UINT64 alignment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
		return (size + (alignment - 1)) & ~(alignment - 1);
	}

	UINT64 Raytracing::CopyMem(void* dest, const void* data, UINT64 size)
	{
		memcpy(dest, data, size);

		return size;
	}

	D3D12_DISPATCH_RAYS_DESC Raytracing::MakeDispatchRaysDesc(UINT rayGenIndex, UINT width, UINT height, UINT depth) const
	{
		if (rayGenIndex >= rayGenNum_) {
			throw runtime_error("[Raytracing::MakeDispatchRaysDesc] rayGenIndex is out of range");
		}

		auto startAddress = sbtBuffer_->GetGPUAddress();
		D3D12_DISPATCH_RAYS_DESC raysDesc = {};
		raysDesc.RayGenerationShaderRecord.StartAddress = startAddress + eachRayGenSize_ * rayGenIndex;
		raysDesc.RayGenerationShaderRecord.SizeInBytes = eachRayGenSize_;
		startAddress += sumRayGenRegionSize_;

		raysDesc.MissShaderTable.StartAddress = startAddress;
		raysDesc.MissShaderTable.SizeInBytes = sumMissRegionSize_;
		raysDesc.MissShaderTable.StrideInBytes = eachMissSize_;
		startAddress += sumMissRegionSize_;

		raysDesc.HitGroupTable.StartAddress = startAddress;
		raysDesc.HitGroupTable.SizeInBytes = sumHitGroupRegionSize_;
		raysDesc.HitGroupTable.StrideInBytes = eachHitGroupSize_;
		startAddress += sumHitGroupRegionSize_;

		raysDesc.Width = width;
		raysDesc.Height = height;
		raysDesc.Depth = depth;

		return raysDesc;
	}

	Raytracing::Raytracing(const Device& device, std::wstring name, const StateObjectHandle& stateObject,
		std::vector<std::vector<LocalRootArguments>> rayGenLocalRootArguments,
		std::vector<std::vector<LocalRootArguments>> missLocalRootArguments,
		std::vector<std::vector<LocalRootArguments>> hitGroupLocalRootArguments
	)
		: name_(name)
	{
		stateObject->GetStateObject()->QueryInterface(IID_PPV_ARGS(soProp_.ReleaseAndGetAddressOf()));

		if (stateObject->GetStateObjectType() == StateObjectType::WorkGraph) {
			throw runtime_error("[Raytracing::Raytracing] stateObject is not a raytracing state object");
		}
		
		const auto& rayGens = stateObject->GetRayGens();
		rayGenNum_ = static_cast<UINT>(rayGens.size());
		const auto& misses = stateObject->GetMisses();
		const auto& hitGroups = stateObject->GetHitGroups();

		if (rayGenLocalRootArguments.empty())
		{
			rayGenLocalRootArguments.resize(rayGens.size());
		}
		if (missLocalRootArguments.empty())
		{
			missLocalRootArguments.resize(misses.size());
		}
		if (hitGroupLocalRootArguments.empty())
		{
			hitGroupLocalRootArguments.resize(hitGroups.size());
		}

		if (rayGenLocalRootArguments.size() != rayGens.size()) {
			throw runtime_error("[Raytracing::Raytracing] rayGenLocalRootArguments size does not match rayGens size (pass {} if no ray generation shader uses local root arguments)");
		}
		if (misses.size() != missLocalRootArguments.size()) {
			throw runtime_error("[Raytracing::Raytracing] missLocalRootArguments size does not match misses size (pass {} if no miss shader uses local root arguments)");
		}
		if (hitGroups.size() != hitGroupLocalRootArguments.size()) {
			throw runtime_error("[Raytracing::Raytracing] hitGroupLocalRootArguments size does not match hitGroups size (pass {} if no hit group uses local root arguments)");
		}

		// All record aligned max record size
		UINT64 maxRayGenSize = 0;
		for (const auto& rayGen : rayGens) {
			UINT64 shaderSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			UINT64 localRootSigSize = 0;
			if (rayGen.localRootSignature_) {
				localRootSigSize = rayGen.localRootSignature_->GetSize();
			}
			UINT64 rayGenSize = shaderSize + localRootSigSize;
			maxRayGenSize = max(maxRayGenSize, rayGenSize);
		}
		// RayGen record size must be aligned to 64 (= D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT)
		eachRayGenSize_ = AlignForSBT(maxRayGenSize);
		sumRayGenRegionSize_ = AlignForSBT(eachRayGenSize_ * rayGens.size());

		UINT64 maxMissSize = 0;
		for (const auto& miss : misses) {
			UINT64 shaderSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			UINT64 localRootSigSize = 0;
			if (miss.localRootSignature_) {
				localRootSigSize = miss.localRootSignature_->GetSize();
			}
			UINT64 missSize = shaderSize + localRootSigSize;
			maxMissSize = max(maxMissSize, missSize);
		}
		eachMissSize_ = AlignForSBTRecord(maxMissSize);
		sumMissRegionSize_ = AlignForSBT(eachMissSize_ * misses.size());

		UINT64 maxHitGroupSize = 0;
		for (const auto&  hitGroup : hitGroups) {
			UINT64 shaderSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			UINT64 localRootSigSize = 0;
			if (hitGroup.localRootSignature_) {
				localRootSigSize = hitGroup.localRootSignature_->GetSize();
			}
			UINT64 hitGroupSize = shaderSize + localRootSigSize;
			maxHitGroupSize = max(maxHitGroupSize, hitGroupSize);
		}
		eachHitGroupSize_ = AlignForSBTRecord(maxHitGroupSize);
		sumHitGroupRegionSize_ = AlignForSBT(eachHitGroupSize_ * hitGroups.size());

		UINT64 allRegionSize = sumRayGenRegionSize_ + sumMissRegionSize_ + sumHitGroupRegionSize_;

		sbtBuffer_ = device.CreateBuffer(name_ + L"_sbt", BufferType::Upload, allRegionSize, 1);

		void* rawPtr = sbtBuffer_->Map();
		// uint8_t is 8 bit = 1 byte data
		// This is suitable for data which is per byte
		if (rawPtr) {
			uint8_t* pSBT = static_cast<uint8_t*>(rawPtr);

			auto pRayGen = pSBT + 0;
			for (int i = 0; i < rayGens.size(); i++) {
				auto pThisRayGen = pRayGen + i * eachRayGenSize_;
				// NOTE : If you use index, you may be ought to use vector not initializer
				void* id = soProp_->GetShaderIdentifier(rayGens[i].shader_->GetEntryName().c_str());
				CopyMem(pThisRayGen, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				// local root descriptor pointer
				if (rayGens[i].localRootSignature_) {
					const auto& localRootArguments = rayGenLocalRootArguments[i];
					const auto& localRootSignature = rayGens[i].localRootSignature_;
					if (localRootArguments.size() != localRootSignature->GetRootParametersVec().size()) {
						throw runtime_error("[Raytracing::Raytracing] rayGenLocalRootArguments[" + to_string(i) + "] size does not match the number of root parameters of its local root signature");
					}
					for (UINT j = 0; j < localRootArguments.size(); j++) {
						localRootSignature->ValidateRootArgument(j, localRootArguments[j]);
					}
					RootSignature::WriteLocalRootArguments(pThisRayGen + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, localRootArguments);
				}
				else {
					if (rayGenLocalRootArguments[i].size() > 0) {
						throw runtime_error("[Raytracing::Raytracing] rayGenLocalRootArguments[" + to_string(i) + "] is not empty, but rayGens[" + to_string(i) + "] does not have a local root signature");
					}
				}
			}

			auto pMiss = pSBT + sumRayGenRegionSize_;
			for (int i = 0; i < misses.size(); i++) {
				auto pThisMiss = pMiss + i * eachMissSize_;

				// NOTE : If you use index, you may be ought to use vector not initializer ?
				void* id = soProp_->GetShaderIdentifier(misses[i].shader_->GetEntryName().c_str());
				CopyMem(pThisMiss, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				// local root descriptor pointer
				if (misses[i].localRootSignature_)
				{
					const auto& localRootArguments = missLocalRootArguments[i];
					const auto& localRootSignature = misses[i].localRootSignature_;
					if (localRootArguments.size() != localRootSignature->GetRootParametersVec().size()) {
						throw runtime_error("[Raytracing::Raytracing] missLocalRootArguments[" + to_string(i) + "] size does not match the number of root parameters of its local root signature");
					}
					for (UINT j = 0; j < localRootArguments.size(); j++) {
						localRootSignature->ValidateRootArgument(j, localRootArguments[j]);
					}
					RootSignature::WriteLocalRootArguments(pThisMiss + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, localRootArguments);
				}
				else {
					if (missLocalRootArguments[i].size() > 0) {
						throw runtime_error("[Raytracing::Raytracing] missLocalRootArguments[" + to_string(i) + "] is not empty, but misses[" + to_string(i) + "] does not have a local root signature");
					}
				}
			}

			auto pHitGroup = pSBT + sumRayGenRegionSize_ + sumMissRegionSize_;
			for (int i = 0; i < hitGroups.size(); i++) {
				auto pThisHitGroup = pHitGroup + i * eachHitGroupSize_;

				// NOTE : If you use index, you may be ought to use vector not initializer ?
				void* id = soProp_->GetShaderIdentifier(hitGroups[i].groupName_.c_str());
				CopyMem(pThisHitGroup, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				// local root descriptor pointer
				if (hitGroups[i].localRootSignature_)
				{
					const auto& localRootArguments = hitGroupLocalRootArguments[i];
					const auto& localRootSignature = hitGroups[i].localRootSignature_;
					if (localRootArguments.size() != localRootSignature->GetRootParametersVec().size()) {
						throw runtime_error("[Raytracing::Raytracing] hitGroupLocalRootArguments[" + to_string(i) + "] size does not match the number of root parameters of its local root signature");
					}
					for (UINT j = 0; j < localRootArguments.size(); j++) {
						localRootSignature->ValidateRootArgument(j, localRootArguments[j]);
					}
					RootSignature::WriteLocalRootArguments(pThisHitGroup + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, localRootArguments);
				}
				else {
					if (hitGroupLocalRootArguments[i].size() > 0) {
						throw runtime_error("[Raytracing::Raytracing] hitGroupLocalRootArguments[" + to_string(i) + "] is not empty, but hitGroups[" + to_string(i) + "] does not have a local root signature");
					}
				}
			}
		}
	}
}