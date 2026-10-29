#include "AS.hpp"

#include "Command.hpp"
#include "Device.hpp"
#include "Resource.hpp"
#include "Mesh.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	void BLAS::CreateBLAS(const Device& device, const MeshHandle& mesh, const CommandHandle& command)
	{
		D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
		geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
		geomDesc.Triangles.VertexBuffer.StartAddress = mesh->GetVertexBuffer()->GetGPUAddress();
		geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
		geomDesc.Triangles.VertexCount = mesh->GetVertexCount();
		geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geomDesc.Triangles.IndexBuffer = mesh->GetIndexBuffer()->GetGPUAddress();
		geomDesc.Triangles.IndexCount = mesh->GetNumIndices();
		geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs = {};
		buildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		buildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		buildInputs.NumDescs = 1;
		buildInputs.pGeometryDescs = &geomDesc;
		buildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
		device.GetStableDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&buildInputs, &prebuildInfo);

		cout << "BLAS scratchBuffer size = " << prebuildInfo.ScratchDataSizeInBytes << endl;
		// CreateBuffer take strideSize and numElements as UINT, so we need to avoid overflow.
		// Specifying strideSize = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT (256),
		// scratchSize can support 256 (stride size) x UINT_MAX (elements num) = about 1TB
		UINT64 scratchSize = prebuildInfo.ScratchDataSizeInBytes;
		UINT strideSize = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
		UINT numElements = static_cast<UINT>((scratchSize + strideSize - 1) / strideSize);
		scratchBuffer_ = device.CreateBuffer(name_ + L"_BLAS_scratch", BufferType::Unordered, strideSize, numElements);

		cout << "BLAS ASBuffer size = " << prebuildInfo.ResultDataMaxSizeInBytes << endl;
		ASBuffer_ = device.CreateAS(name_ + L"_BLAS", prebuildInfo.ResultDataMaxSizeInBytes);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildASDesc = {};
		buildASDesc.Inputs = buildInputs;
		buildASDesc.DestAccelerationStructureData = ASBuffer_->GetResource()->GetGPUVirtualAddress();
		buildASDesc.ScratchAccelerationStructureData = scratchBuffer_->GetResource()->GetGPUVirtualAddress();

		auto transBarrier = CD3DX12_RESOURCE_BARRIER::Transition(scratchBuffer_->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		command->GetStableCommandList()->ResourceBarrier(1, &transBarrier);
		command->GetStableCommandList()->BuildRaytracingAccelerationStructure(&buildASDesc, 0, nullptr);

		auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(ASBuffer_->GetResource().Get());
		command->GetStableCommandList()->ResourceBarrier(1, &barrier);
		command->WaitCommand();
	}

	BLAS::BLAS(const Device& device, wstring name, const CommandHandle& command, const MeshHandle& mesh)
		: name_(name)
	{
		CreateBLAS(device, mesh, command);
	}

	D3D12_GPU_VIRTUAL_ADDRESS BLAS::GetASAddress() const
	{
		return ASBuffer_->GetResource()->GetGPUVirtualAddress();
	}

	void TLAS::CreateTLAS(const Device& device, const CommandHandle& command)
	{
		vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDesc;
		instanceDesc.resize(tlasDescs_.size());
		for (size_t numDescs = 0; numDescs < tlasDescs_.size(); numDescs++) {
			D3D12_RAYTRACING_INSTANCE_DESC desc = {};
			XMFLOAT4X4 transform;
			XMStoreFloat4x4(&transform, tlasDescs_[numDescs].transform_);
			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 4; j++) {
					instanceDesc[numDescs].Transform[i][j] = transform.m[i][j];
				}
			}
			instanceDesc[numDescs].InstanceMask = tlasDescs_[numDescs].instanceMask_;
			instanceDesc[numDescs].AccelerationStructure = tlasDescs_[numDescs].blas_->GetASAddress();
			instanceDesc[numDescs].Flags = tlasDescs_[numDescs].flags_;
		}

		shared_ptr<Buffer> uploadBuffer;
		uploadBuffer = device.CreateBuffer(L"_upload", BufferType::Upload, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), static_cast<UINT>(instanceDesc.size()));
		void* rawPtr = uploadBuffer->Map();
		if (rawPtr) {
			D3D12_RAYTRACING_INSTANCE_DESC* pDesc = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(rawPtr);
			memcpy(pDesc, instanceDesc.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDesc.size());
			uploadBuffer->Unmap();
		}
		cout << "TLAS instanceDescBuffer size = " << sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDesc.size() << endl;
		instanceDescBuffer_ = device.CreateBuffer(name_ + L"_TLAS_instanceDesc", BufferType::Unordered, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), static_cast<UINT>(instanceDesc.size()));

		command->CopyBuffer(uploadBuffer, instanceDescBuffer_);
		command->Transition(instanceDescBuffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		command->WaitCommand();

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs = {};
		buildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		buildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
		buildInputs.NumDescs = static_cast<UINT>(instanceDesc.size());
		buildInputs.InstanceDescs = instanceDescBuffer_->GetResource()->GetGPUVirtualAddress();
		buildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
		device.GetStableDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&buildInputs, &prebuildInfo);

		cout << "TLAS scratchBuffer size = " << prebuildInfo.ScratchDataSizeInBytes << endl;
		// CreateBuffer take strideSize and numElements as UINT, so we need to avoid overflow.
		// Specifying strideSize = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT (256),
		// scratchSize can support 256 (stride size) x UINT_MAX (elements num) = about 1TB
		UINT64 scratchSize = prebuildInfo.ScratchDataSizeInBytes;
		UINT strideSize = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
		UINT numElements = static_cast<UINT>((scratchSize + strideSize - 1) / strideSize);
		scratchBuffer_ = device.CreateBuffer(name_ + L"_TLAS_scratch", BufferType::Unordered, strideSize, numElements);

		cout << "TLAS ASBuffer size = " << prebuildInfo.ResultDataMaxSizeInBytes << endl;
		ASBuffer_ = device.CreateAS(name_ + L"_TLAS", prebuildInfo.ResultDataMaxSizeInBytes);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildASDesc = {};
		buildASDesc.Inputs = buildInputs;
		buildASDesc.DestAccelerationStructureData = ASBuffer_->GetResource()->GetGPUVirtualAddress();
		buildASDesc.ScratchAccelerationStructureData = scratchBuffer_->GetResource()->GetGPUVirtualAddress();

		command->GetStableCommandList()->BuildRaytracingAccelerationStructure(&buildASDesc, 0, nullptr);

		auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(ASBuffer_->GetResource().Get());
		command->GetStableCommandList()->ResourceBarrier(1, &barrier);
		command->WaitCommand();
	}

	TLAS::TLAS(const Device& device, wstring name, const CommandHandle& command, const vector<TLASDesc>& tlasDescs)
		: name_(name), tlasDescs_(tlasDescs)
	{
		CreateTLAS(device, command);
	}

	shared_ptr<AS> TLAS::GetASBuffer() const
	{
		return ASBuffer_;
	}

	D3D12_GPU_VIRTUAL_ADDRESS TLAS::GetASAddress() const
	{
		return ASBuffer_->GetResource()->GetGPUVirtualAddress();
	}

}