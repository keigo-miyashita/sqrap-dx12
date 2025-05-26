#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

bool BLAS::CreateBLAS(const ASMesh& mesh, Command& command, Fence& fence, std::wstring name)
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
	if (!scratchBuffer_.Init(
		pDevice_, (UINT)prebuildInfo.ScratchDataSizeInBytes, 1,
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	))
	{
		cerr << "Failed to create scratch buffer of BLAS class" << endl;
		return false;
	}

	cout << "BLAS ASBuffer size = " << prebuildInfo.ResultDataMaxSizeInBytes << endl;
	if (!ASBuffer_.Init(
		pDevice_, (UINT)prebuildInfo.ResultDataMaxSizeInBytes, 1,
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE
	))
	{
		cerr << "Failed to create AS buffer of BLAS class" << endl;
		return false;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildASDesc = {};
	buildASDesc.Inputs = buildInputs;
	buildASDesc.DestAccelerationStructureData = ASBuffer_.GetResource()->GetGPUVirtualAddress();
	buildASDesc.ScratchAccelerationStructureData = scratchBuffer_.GetResource()->GetGPUVirtualAddress();

	auto transBarrier = CD3DX12_RESOURCE_BARRIER::Transition(scratchBuffer_.GetResource().Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	command.GetStableCommandList()->ResourceBarrier(1, &transBarrier);
	command.GetStableCommandList()->BuildRaytracingAccelerationStructure(&buildASDesc, 0, nullptr);

	auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(ASBuffer_.GetResource().Get());
	command.GetStableCommandList()->ResourceBarrier(1, &barrier);
	fence.WaitCommand(command);

	return true;
}

BLAS::BLAS()
{

}

bool BLAS::Init(Device* pDevice, const ASMesh& mesh, Command& command, Fence& fence, std::wstring name)
{
	pDevice_ = pDevice;
	if (pDevice_ == nullptr) {
		cerr << "BLAS class pDevice doesn't have any pounter" << endl;
	}
	if (!CreateBLAS(mesh, command, fence, name)) {
		return false;
	}

	return true;
}

D3D12_GPU_VIRTUAL_ADDRESS BLAS::GetASAddress()
{
	return ASBuffer_.GetResource()->GetGPUVirtualAddress();
}

bool TLAS::CreateTLAS(Command& command, Fence& fence, std::wstring name)
{
	vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDesc;
	instanceDesc.resize(tlasDescs_.size());
	for (int numDescs = 0; numDescs < tlasDescs_.size(); numDescs++) {
		D3D12_RAYTRACING_INSTANCE_DESC desc = {};
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 4; j++) {
				instanceDesc[numDescs].Transform[i][j] = tlasDescs_[numDescs].transform.m[i][j];
			}
		}
		instanceDesc[numDescs].InstanceMask = 0xFF;
		instanceDesc[numDescs].AccelerationStructure = tlasDescs_[numDescs].blas->GetASAddress();
		instanceDesc[numDescs].Flags = tlasDescs_[numDescs].flags;
	}

	Buffer uploadBuffer;
	uploadBuffer.InitAsUpload(pDevice_, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), instanceDesc.size(), name);
	void* rawPtr = uploadBuffer.Map();
	if (rawPtr) {
		D3D12_RAYTRACING_INSTANCE_DESC* pDesc = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(rawPtr);
		memcpy(pDesc, instanceDesc.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDesc.size());
		uploadBuffer.Unmap();
	}
	cout << "uploadBuffer result : stride = " << uploadBuffer.GetResource()->GetDesc().Width << " num = " << uploadBuffer.GetResource()->GetDesc().Height << endl;
	cout << "TLAS instanceDescBuffer size = " << sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDesc.size() << endl;
	if (!instanceDescBuffer_.Init(
		pDevice_, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), instanceDesc.size(),
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, name
	))
	{
		cerr << "Failed to create TLAS instanceDescBuffer" << endl;
		return false;
	}
	cout << "instanceDescBuffer_r result : stride = " << instanceDescBuffer_.GetResource()->GetDesc().Width << " num = " << instanceDescBuffer_.GetResource()->GetDesc().Height << endl;

	command.CopyBuffer(uploadBuffer, instanceDescBuffer_);
	fence.WaitCommand(command);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs = {};
	buildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	buildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	buildInputs.NumDescs = instanceDesc.size();
	buildInputs.InstanceDescs = instanceDescBuffer_.GetResource()->GetGPUVirtualAddress();
	buildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	pDevice_->GetStableDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&buildInputs, &prebuildInfo);

	cout << "TLAS scratchBuffer size = " << prebuildInfo.ScratchDataSizeInBytes << endl;
	if (!scratchBuffer_.Init(
		pDevice_, prebuildInfo.ScratchDataSizeInBytes, 1,
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	))
	{
		cerr << "Failed to create TLAS scratchBuffer" << endl;
		return false;
	}

	cout << "TLAS ASBuffer size = " << prebuildInfo.ResultDataMaxSizeInBytes << endl;
	if (!ASBuffer_.Init(
		pDevice_, prebuildInfo.ResultDataMaxSizeInBytes, 1,
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE
	))
	{
		cerr << "Failed to create TLAS ASBuffer" << endl;
		return false;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildASDesc = {};
	buildASDesc.Inputs = buildInputs;
	buildASDesc.DestAccelerationStructureData = ASBuffer_.GetResource()->GetGPUVirtualAddress();
	buildASDesc.ScratchAccelerationStructureData = scratchBuffer_.GetResource()->GetGPUVirtualAddress();

	command.GetStableCommandList()->BuildRaytracingAccelerationStructure(&buildASDesc, 0, nullptr);

	auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(ASBuffer_.GetResource().Get());
	command.GetStableCommandList()->ResourceBarrier(1, &barrier);
	fence.WaitCommand(command);

	return true;
}

TLAS::TLAS()
{
	
}

bool TLAS::Init(Device* pDevice, Command& command, Fence& fence, std::vector<TLASDesc> tlasDescs, std::wstring name)
{
	pDevice_ = pDevice;
	if (pDevice_ == nullptr) {
		cerr << "TLAS class pDevice doesn't have any pounter" << endl;
	}
	tlasDescs_ = tlasDescs;
	if (!CreateTLAS(command, fence, name)) {
		return false;
	}

	return true;
}

Buffer TLAS::GetASBuffer() const
{
	return ASBuffer_;
}

D3D12_GPU_VIRTUAL_ADDRESS TLAS::GetASAddress()
{
	return ASBuffer_.GetResource()->GetGPUVirtualAddress();
}
