#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool CreateSBTBuffer(UINT numRecords, UINT localRootSigSize)
{
	UINT shaderIDSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	UINT recordSize = shaderIDSize + localRootSigSize;
	UINT bufferSize = recordSize * numRecords;


}

UINT RayTracing::AlignForSBT(UINT size)
{
	const UINT alignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
	return (size + (alignment - 1)) & ~(alignment - 1);
}

RayTracing::RayTracing(const Device& device, const StateObject& stateObject, std::wstring name = L"")
	: pDevice_(&device), name_(name)
{
	stateObject.GetStateObject()->QueryInterface(IID_PPV_ARGS(soProp_.ReleaseAndGetAddressOf()));

	vector<void*> rayGenIDs;
	vector<void*> missIDs;
	vector<void*> hitGroupIDs;

	for (auto rayGensName : stateObject.GetRayGens()) {
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
	}


}