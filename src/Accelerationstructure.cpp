#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool BLAS::CreateBLAS(const Mesh& mesh, CommandManager& commandManager, Fence& fence, std::wstring name)
{
	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	geomDesc.Triangles.VertexBuffer.StartAddress = mesh.GetVertexBuffer()->GetGPUVirtualAddress();
	geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	geomDesc.Triangles.VertexCount = mesh.GetVertexCount();
	geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	geomDesc.Triangles.IndexBuffer = mesh.GetIndexBuffer()->GetGPUVirtualAddress();
	geomDesc.Triangles.IndexCount = mesh.GetNumIndices();
	geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs = {};
	buildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	buildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	buildInputs.NumDescs = 1;
	buildInputs.pGeometryDescs = &geomDesc;
	buildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	pDevice_->GetStableDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&buildInputs, &prebuildInfo);

	auto scratchBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	auto blasBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	scratchBuffer_.Init(
		pDevice_, prebuildInfo.ScratchDataSizeInBytes, 1,
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);

	ASBuffer_.Init(
		pDevice_, prebuildInfo.ResultDataMaxSizeInBytes, 1,
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildASDesc = {};
	buildASDesc.Inputs = buildInputs;
	buildASDesc.DestAccelerationStructureData = ASBuffer_.GetResource()->GetGPUVirtualAddress();
	buildASDesc.ScratchAccelerationStructureData = scratchBuffer_.GetResource()->GetGPUVirtualAddress();

	commandManager.GetStableCommandList()->BuildRaytracingAccelerationStructure(&buildASDesc, 0, nullptr);

	auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(ASBuffer_.GetResource().Get());
	commandManager.GetStableCommandList()->ResourceBarrier(1, &barrier);
	fence.WaitCommand(commandManager);

	return true;
}

BLAS::BLAS()
{

}

bool BLAS::Init(Device* pDevice, const Mesh& mesh, CommandManager& commandManager, Fence& fence, std::wstring name)
{
	pDevice_ = pDevice;
	if (!CreateBLAS(mesh, commandManager, fence, name)) {
		return false;
	}

	return true;
}

D3D12_GPU_VIRTUAL_ADDRESS BLAS::GetASGPUVirtualAddress() const
{
	return ASBuffer_.GetResource()->GetGPUVirtualAddress();
}

bool TLAS::CreateTLAS(const Mesh& mesh, CommandManager& commandManager, Fence& fence, std::wstring name)
{
	vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDesc;
	for (int numDescs = 0; tlasDescs_.size(); numDescs++) {
		D3D12_RAYTRACING_INSTANCE_DESC desc = {};
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 4; j++) {
				desc.Transform[i][j] = tlasDescs_[numDescs].transform.m[i][j];
			}
		}
		desc.InstanceMask = 0xFF;
		desc.AccelerationStructure = tlasDescs_[numDescs].blas->GetASGPUVirtualAddress();
		desc.Flags = tlasDescs_[numDescs].flags;
	}

	Buffer uploadBuffer;
	uploadBuffer.InitAsUpload(pDevice_, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), instanceDesc.size(), name);
	void* rawPtr = uploadBuffer.Map();
	if (rawPtr) {
		D3D12_RAYTRACING_INSTANCE_DESC* pDesc = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(rawPtr);
		memcpy(pDesc, instanceDesc.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDesc.size());
		uploadBuffer.Unmap();
	}
	instanceDescBuffer_.Init(
		pDevice_, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), instanceDesc.size(),
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, name
	);

	commandManager.CopyBuffer(uploadBuffer, instanceDescBuffer_);
	fence.WaitCommand(commandManager);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs = {};
	buildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	buildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	buildInputs.NumDescs = instanceDesc.size();
	buildInputs.InstanceDescs = instanceDescBuffer_.GetResource()->GetGPUVirtualAddress();
	buildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	pDevice_->GetStableDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&buildInputs, &prebuildInfo);

	scratchBuffer_.Init(
		pDevice_, prebuildInfo.ScratchDataSizeInBytes, 1,
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE
	);

	ASBuffer_.Init(
		pDevice_, prebuildInfo.ResultDataMaxSizeInBytes, 1,
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE
	);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildASDesc = {};
	buildASDesc.Inputs = buildInputs;
	buildASDesc.DestAccelerationStructureData = ASBuffer_.GetResource()->GetGPUVirtualAddress();
	buildASDesc.ScratchAccelerationStructureData = scratchBuffer_.GetResource()->GetGPUVirtualAddress();

	commandManager.GetStableCommandList()->BuildRaytracingAccelerationStructure(&buildASDesc, 0, nullptr);

	auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(ASBuffer_.GetResource().Get());
	commandManager.GetStableCommandList()->ResourceBarrier(1, &barrier);
	fence.WaitCommand(commandManager);

	return true;
}

TLAS::TLAS()
{
	
}

bool TLAS::Init(Device* pDevice, const Mesh& mesh, CommandManager& commandManager, Fence& fence, std::wstring name)
{
	pDevice_ = pDevice;
	if (!CreateTLAS(mesh, commandManager, fence, name)) {
		return false;
	}

	return true;
}
