#include <common.hpp>

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

RayTracing::RayTracing(const Device& device, const StateObject& stateObject, std::wstring name)
	: pDevice_(&device), name_(name)
{
	stateObject.GetStateObject()->QueryInterface(IID_PPV_ARGS(soProp_.ReleaseAndGetAddressOf()));

	vector<void*> rayGenIDs;
	vector<void*> missIDs;
	vector<void*> hitGroupIDs;

	if (std::holds_alternative<StateObjectDesc::RayTracingDesc>(stateObject.GetStateObjectDesc().typeDesc)) {
		StateObjectDesc::RayTracingDesc rtDesc = std::get<StateObjectDesc::RayTracingDesc>(stateObject.GetStateObjectDesc().typeDesc);

		// Align size to max record size
		// Allrecord aligned max record size
		UINT maxRayGenSize = 0;
		for (auto rayGen : rtDesc.rayGens) {
			void* id = soProp_->GetShaderIdentifier(rayGen.shader->GetEntryName().c_str());
			UINT shaderSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			UINT localRootSigSize = rayGen.localRootSig->GetSize();
			UINT rayGenSize = shaderSize + localRootSigSize;
			maxRayGenSize = max(maxRayGenSize, rayGenSize);
		}
		UINT alignedRayGenSize = AlignForSBTRecord(maxRayGenSize);
		UINT rayGenRegionSize = alignedRayGenSize * rtDesc.rayGens.size();

		UINT maxMissSize = 0;
		for (auto miss : rtDesc.misses) {
			void* id = soProp_->GetShaderIdentifier(miss.shader->GetEntryName().c_str());
			UINT shaderSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			UINT localRootSigSize = miss.localRootSig->GetSize();
			UINT missSize = shaderSize + localRootSigSize;
			maxMissSize = max(maxMissSize, missSize);
		}
		UINT alignedMissSize = AlignForSBTRecord(maxMissSize);
		UINT missRegionSize = alignedMissSize * rtDesc.misses.size();

		UINT maxHitGroupSize = 0;
		for (auto hitGroup : rtDesc.hitGroups) {
			void* id = soProp_->GetShaderIdentifier(hitGroup.groupName.c_str());
			UINT shaderSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			UINT localRootSigSize = hitGroup.localRootSig->GetSize();
			UINT hitGroupSize = shaderSize + localRootSigSize;
			maxHitGroupSize = max(maxHitGroupSize, hitGroupSize);
		}
		UINT alignedHitGroupSize = AlignForSBTRecord(maxHitGroupSize);
		UINT hitGroupRegionSize = alignedHitGroupSize * rtDesc.hitGroups.size();

		UINT allRegionSize = rayGenRegionSize + missRegionSize + hitGroupRegionSize;
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
			for (auto rayGen : rtDesc.rayGens) {
				void* id = soProp_->GetShaderIdentifier(rayGen.shader->GetEntryName().c_str());
				// local root descriptor pointer
			}

			auto pMiss = pRayGen + rayGenRegionSize;
			for (auto miss : rtDesc.misses) {
				void* id = soProp_->GetShaderIdentifier(miss.shader->GetEntryName().c_str());
			}

			auto pHitGroup = pMiss + missRegionSize;
			for (auto hitGroup : rtDesc.hitGroups) {
				void* id = soProp_->GetShaderIdentifier(hitGroup.groupName.c_str());
			}
		}
	}

	/*for (auto rayGensName : stateObject.GetStateObjectDesc()) {
		void* id = soProp_->GetShaderIdentifier(rayGensName.c_str());
		rayGenIDs.push_back(id);
	}

	for (auto missName : stateObject.GetMisses()) {
		void* id = soProp_->GetShaderIdentifier(missName.c_str());
		missIDs.push_back(id);
	}

	for (auto hitGroupName : stateObject.GetHitGroups()) {
		void* id = soProp_->GetShaderIdentifier(hitGroupName.c_str());
		hitGroupIDs.push_back(id);
	}*/


}