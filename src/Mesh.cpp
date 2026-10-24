#include "Mesh.hpp"

#include "Command.hpp"
#include "Device.hpp"
#include "GLTFLoader.hpp"
#include "Resource.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	wstring StringToWString(const string& str)
	{
		int size_needed = MultiByteToWideChar(CP_UTF8, 0,
			str.c_str(), (int)str.size(),
			nullptr, 0);

		wstring result(size_needed, 0);

		MultiByteToWideChar(CP_UTF8, 0,
			str.c_str(), (int)str.size(),
			&result[0], size_needed);

		return result;
	}

	void Mesh::CreateVertexBuffer(const Device& device, const CommandHandle& command, const std::vector<Vertex>& vertices)
	{
		vertexCount_ = static_cast<UINT>(vertices.size());

		shared_ptr<Buffer> vertexUploadBuffer;
		vertexUploadBuffer = device.CreateBuffer(name_ + L"_upload_Vertex", BufferType::Upload, sizeof(Vertex), vertices.size());
		void* rawPtr = vertexUploadBuffer->Map();
		if (rawPtr) {
			Vertex* pVertex = static_cast<Vertex*>(rawPtr);
			memcpy(pVertex, vertices.data(), sizeof(Vertex) * vertices.size());
			vertexUploadBuffer->Unmap();
		}

		vertexBuffer_ = device.CreateBuffer(name_ + L"_Vertex", BufferType::Default, sizeof(Vertex), vertices.size());

		command->CopyBuffer(vertexUploadBuffer, vertexUploadBuffer->GetInitialState(), vertexBuffer_, vertexBuffer_->GetInitialState());
		command->WaitCommand();
		vbView_.BufferLocation = vertexBuffer_->GetGPUAddress();
		vbView_.SizeInBytes = vertices.size() * sizeof(Vertex);
		vbView_.StrideInBytes = sizeof(Vertex);
	}

	void Mesh::CreateIndexBuffer(const Device& device, const CommandHandle& command, const std::vector<uint32_t>& indices)
	{
		indexCount_ = static_cast<UINT>(indices.size());

		shared_ptr<Buffer> indexUploadBuffer;
		indexUploadBuffer = device.CreateBuffer(name_ + L"_upload_Index", BufferType::Upload, sizeof(uint32_t), indices.size());
		void* rawPtr = indexUploadBuffer->Map();
		if (rawPtr) {
			uint32_t* pIndex = static_cast<uint32_t*>(rawPtr);
			memcpy(pIndex, indices.data(), sizeof(uint32_t) * indices.size());
			indexUploadBuffer->Unmap();
		}

		indexBuffer_ = device.CreateBuffer(name_ + L"_Index", BufferType::Default, sizeof(uint32_t), indices.size());

		command->CopyBuffer(indexUploadBuffer, indexUploadBuffer->GetInitialState(), indexBuffer_, indexBuffer_->GetInitialState());
		command->WaitCommand();
		ibView_.BufferLocation = indexBuffer_->GetGPUAddress();
		ibView_.SizeInBytes = indices.size() * sizeof(uint32_t);
		ibView_.Format = DXGI_FORMAT_R32_UINT;
	}

	Mesh::Mesh(const Device& device, std::wstring name, const CommandHandle& command, string modelPath)
		: name_(name)
	{
		GLTFModel gltfMesh = LoadGLTF(modelPath);
		CreateVertexBuffer(device, command, gltfMesh.mesh.vertices);
		CreateIndexBuffer(device, command, gltfMesh.mesh.indices);
		subMeshes_ = gltfMesh.mesh.subMeshes;
	}

	Mesh::Mesh(const Device& device, std::wstring name, const CommandHandle& command, const vector<Vertex>& vertices, const vector<uint32_t>& indices)
		: name_(name)
	{
		CreateVertexBuffer(device, command, vertices);
		CreateIndexBuffer(device, command, indices);
		subMeshes_.push_back({ 0, static_cast<UINT>(indices.size()), 0, -1 });
	}

	BufferHandle Mesh::GetVertexBuffer() const
	{
		return vertexBuffer_;
	}

	D3D12_VERTEX_BUFFER_VIEW Mesh::GetVBView() const
	{
		return vbView_;
	}

	const D3D12_VERTEX_BUFFER_VIEW* Mesh::GetVBViewPtr() const
	{
		return &vbView_;
	}

	UINT Mesh::GetVertexCount() const
	{
		return vertexCount_;
	}

	BufferHandle Mesh::GetIndexBuffer() const
	{
		return indexBuffer_;
	}

	D3D12_INDEX_BUFFER_VIEW Mesh::GetIBView() const
	{
		return ibView_;
	}

	const D3D12_INDEX_BUFFER_VIEW* Mesh::GetIBViewPtr() const
	{
		return &ibView_;
	}

	UINT Mesh::GetNumIndices() const
	{
		return indexCount_;
	}

	const std::vector<SubMesh>& Mesh::GetSubMeshes() const
	{
		return subMeshes_;
	}
}