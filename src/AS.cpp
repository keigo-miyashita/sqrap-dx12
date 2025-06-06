#include "AS.hpp"

#include "Command.hpp"
#include "Device.hpp"
#include "Resource.hpp"
#include "Mesh.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool BLAS::CreateBLAS(const ASMesh& mesh)
{
	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	geomDesc.Triangles.VertexBuffer.StartAddress = mesh.GetVertexBuffer().GetGPUAddress();
	geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(ASVertex);
	geomDesc.Triangles.VertexCount = mesh.GetVertexCount();
	geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geomDesc.Triangles.IndexBuffer = mesh.GetIndexBuffer().GetGPUAddress();
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

	cout << "BLAS scratchBuffer size = " << prebuildInfo.ScratchDataSizeInBytes << endl;
	scratchBuffer_ = pDevice_->CreateBuffer(BufferType::Unordered, (UINT)prebuildInfo.ScratchDataSizeInBytes, 1);

	cout << "BLAS ASBuffer size = " << prebuildInfo.ResultDataMaxSizeInBytes << endl;
	ASBuffer_ = pDevice_->CreateAS((UINT)prebuildInfo.ResultDataMaxSizeInBytes);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildASDesc = {};
	buildASDesc.Inputs = buildInputs;
	buildASDesc.DestAccelerationStructureData = ASBuffer_->GetResource()->GetGPUVirtualAddress();
	buildASDesc.ScratchAccelerationStructureData = scratchBuffer_->GetResource()->GetGPUVirtualAddress();

	auto transBarrier = CD3DX12_RESOURCE_BARRIER::Transition(scratchBuffer_->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	command_->GetStableCommandList()->ResourceBarrier(1, &transBarrier);
	command_->GetStableCommandList()->BuildRaytracingAccelerationStructure(&buildASDesc, 0, nullptr);

	auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(ASBuffer_->GetResource().Get());
	command_->GetStableCommandList()->ResourceBarrier(1, &barrier);
	command_->WaitCommand();

	return true;
}

BLAS::BLAS(const Device& device, std::shared_ptr<Command> command, const ASMesh& mesh, std::wstring name)
	: pDevice_(&device), command_(command), name_(name)
{
	CreateBLAS(mesh);
}

D3D12_GPU_VIRTUAL_ADDRESS BLAS::GetASAddress()
{
	return ASBuffer_->GetResource()->GetGPUVirtualAddress();
}

bool TLAS::CreateTLAS()
{
	vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDesc;
	instanceDesc.resize(tlasDescs_.size());
	for (int numDescs = 0; numDescs < tlasDescs_.size(); numDescs++) {
		D3D12_RAYTRACING_INSTANCE_DESC desc = {};
		XMFLOAT4X4 transform;
		XMStoreFloat4x4(&transform, tlasDescs_[numDescs].transform);
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 4; j++) {
				instanceDesc[numDescs].Transform[i][j] = transform.m[i][j];
			}
		}
		instanceDesc[numDescs].InstanceMask = 0xFF;
		instanceDesc[numDescs].AccelerationStructure = tlasDescs_[numDescs].blas->GetASAddress();
		instanceDesc[numDescs].Flags = tlasDescs_[numDescs].flags;
	}

	shared_ptr<Buffer> uploadBuffer;
	uploadBuffer = pDevice_->CreateBuffer(BufferType::Upload, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), instanceDesc.size());
	void* rawPtr = uploadBuffer->Map();
	if (rawPtr) {
		D3D12_RAYTRACING_INSTANCE_DESC* pDesc = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(rawPtr);
		memcpy(pDesc, instanceDesc.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDesc.size());
		uploadBuffer->Unmap();
	}
	cout << "uploadBuffer result : stride = " << uploadBuffer->GetResource()->GetDesc().Width << " num = " << uploadBuffer->GetResource()->GetDesc().Height << endl;
	cout << "TLAS instanceDescBuffer size = " << sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDesc.size() << endl;
	instanceDescBuffer_ = pDevice_->CreateBuffer(BufferType::Unordered, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), instanceDesc.size());

	command_->CopyBuffer(*uploadBuffer, *instanceDescBuffer_);
	command_->WaitCommand();

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs = {};
	buildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	buildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	buildInputs.NumDescs = instanceDesc.size();
	buildInputs.InstanceDescs = instanceDescBuffer_->GetResource()->GetGPUVirtualAddress();
	buildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	pDevice_->GetStableDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&buildInputs, &prebuildInfo);

	cout << "TLAS scratchBuffer size = " << prebuildInfo.ScratchDataSizeInBytes << endl;
	scratchBuffer_ = pDevice_->CreateBuffer(BufferType::Unordered, prebuildInfo.ScratchDataSizeInBytes, 1);

	cout << "TLAS ASBuffer size = " << prebuildInfo.ResultDataMaxSizeInBytes << endl;
	ASBuffer_ = pDevice_->CreateAS(prebuildInfo.ResultDataMaxSizeInBytes);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildASDesc = {};
	buildASDesc.Inputs = buildInputs;
	buildASDesc.DestAccelerationStructureData = ASBuffer_->GetResource()->GetGPUVirtualAddress();
	buildASDesc.ScratchAccelerationStructureData = scratchBuffer_->GetResource()->GetGPUVirtualAddress();

	command_->GetStableCommandList()->BuildRaytracingAccelerationStructure(&buildASDesc, 0, nullptr);

	auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(ASBuffer_->GetResource().Get());
	command_->GetStableCommandList()->ResourceBarrier(1, &barrier);
	command_->WaitCommand();

	return true;
}

TLAS::TLAS(const Device& device, std::shared_ptr<Command> command, const std::vector<TLASDesc>& tlasDescs, std::wstring name)
	: pDevice_(&device), command_(command), tlasDescs_(tlasDescs), name_(name)
{
	CreateTLAS();
}

std::shared_ptr<AS> TLAS::GetASBuffer() const
{
	return ASBuffer_;
}

D3D12_GPU_VIRTUAL_ADDRESS TLAS::GetASAddress()
{
	return ASBuffer_->GetResource()->GetGPUVirtualAddress();
}
