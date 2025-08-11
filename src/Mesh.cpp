#include "Mesh.hpp"

#include "Command.hpp"
#include "Device.hpp"
#include "Resource.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tiny_gltf.h>

namespace sqrp
{
	bool Mesh::LoadModel(std::string modelPath, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		string err, warn;

		bool success = loader.LoadASCIIFromFile(&model, &err, &warn, modelPath.c_str());
		if (!success) {
			cerr << "failed to load gltf\n";
			assert(0);
			return false;
		}

		vertices.clear();
		indices.clear();

		for (auto& mesh : model.meshes) {
			for (auto& primitive : mesh.primitives) {
				int posAccessorIndex = primitive.attributes["POSITION"];
				const tinygltf::Accessor& posAccessor = model.accessors[posAccessorIndex];
				const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
				tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
				float* positions = reinterpret_cast<float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);

				int normalAccessorIndex = primitive.attributes["NORMAL"];
				const tinygltf::Accessor& normalAccessor = model.accessors[normalAccessorIndex];
				const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor.bufferView];
				tinygltf::Buffer& normalBuffer = model.buffers[normalBufferView.buffer];
				float* normals = reinterpret_cast<float*>(&normalBuffer.data[normalBufferView.byteOffset + normalAccessor.byteOffset]);

				int tangentAccessorIndex = primitive.attributes["TANGENT"];
				const tinygltf::Accessor& tangentAccessor = model.accessors[tangentAccessorIndex];
				const tinygltf::BufferView& tangentBufferView = model.bufferViews[tangentAccessor.bufferView];
				tinygltf::Buffer& tangentBuffer = model.buffers[tangentBufferView.buffer];
				float* tangents = reinterpret_cast<float*>(&tangentBuffer.data[tangentBufferView.byteOffset + tangentAccessor.byteOffset]);

				int uvAccessorIndex = primitive.attributes["TEXCOORD_0"];
				const tinygltf::Accessor& uvAccessor = model.accessors[uvAccessorIndex];
				const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor.bufferView];
				tinygltf::Buffer& uvBuffer = model.buffers[uvBufferView.buffer];
				float* uvs = reinterpret_cast<float*>(&uvBuffer.data[uvBufferView.byteOffset + uvAccessor.byteOffset]);

				for (int i = 0; i < posAccessor.count; i++) {
					vertices.push_back(
						{
							(XMFLOAT4(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2], 1.0f)),
							(XMFLOAT4(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2], 1.0f)),
							(XMFLOAT4(tangents[i * 4], tangents[i * 4 + 1], tangents[i * 4 + 2], tangents[i * 4 + 3])),
							(XMFLOAT2(uvs[i * 2], uvs[i * 2 + 1]))
						});
				}

				int indicesAccessorIndex = primitive.indices;
				const tinygltf::Accessor& indicesAccessor = model.accessors[indicesAccessorIndex];
				const tinygltf::BufferView& indicesBufferView = model.bufferViews[indicesAccessor.bufferView];
				tinygltf::Buffer& indicesBuffer = model.buffers[indicesBufferView.buffer];
				unsigned short* index = reinterpret_cast<unsigned short*>(&indicesBuffer.data[indicesBufferView.byteOffset + indicesAccessor.byteOffset]);
				for (int i = 0; i < indicesAccessor.count; i++) {
					indices.push_back(index[i]);
				}
			}
		}
		return true;
	}

	HRESULT Mesh::CreateVertexBuffer()
	{
		auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices_.size() * sizeof(Vertex));
		shared_ptr<Buffer> vertexUploadBuffer;
		vertexUploadBuffer = pDevice_->CreateBuffer(BufferType::Upload, sizeof(Vertex), vertices_.size());
		void* rawPtr = vertexUploadBuffer->Map();
		if (rawPtr) {
			Vertex* pVertex = static_cast<Vertex*>(rawPtr);
			memcpy(pVertex, vertices_.data(), sizeof(Vertex) * vertices_.size());
			vertexUploadBuffer->Unmap();
		}

		vertexBuffer_ = pDevice_->CreateBuffer(BufferType::Default, sizeof(Vertex), vertices_.size());
		command_->CopyBuffer(vertexUploadBuffer, vertexBuffer_);
		command_->WaitCommand();
		vbView_.BufferLocation = vertexBuffer_->GetGPUAddress();
		vbView_.SizeInBytes = vertices_.size() * sizeof(Vertex);
		vbView_.StrideInBytes = sizeof(Vertex);

		return S_OK;
	}

	HRESULT Mesh::CreateIndexBuffer()
	{
		auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(indices_.size() * sizeof(uint32_t));
		shared_ptr<Buffer> indexUploadBuffer;
		indexUploadBuffer = pDevice_->CreateBuffer(BufferType::Upload, sizeof(uint32_t), indices_.size());
		void* rawPtr = indexUploadBuffer->Map();
		if (rawPtr) {
			uint32_t* pIndex = static_cast<uint32_t*>(rawPtr);
			memcpy(pIndex, indices_.data(), sizeof(uint32_t) * indices_.size());
			indexUploadBuffer->Unmap();
		}

		indexBuffer_ = pDevice_->CreateBuffer(BufferType::Default, sizeof(uint32_t), indices_.size());
		command_->CopyBuffer(indexUploadBuffer, indexBuffer_);
		command_->WaitCommand();
		ibView_.BufferLocation = indexBuffer_->GetGPUAddress();
		ibView_.SizeInBytes = indices_.size() * sizeof(uint32_t);
		ibView_.Format = DXGI_FORMAT_R32_UINT;

		return S_OK;
	}

	Mesh::Mesh(const Device& device, CommandHandle command, std::string modelPath)
		: pDevice_(&device), command_(command)
	{
		LoadModel(modelPath, vertices_, indices_);
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	Mesh::Mesh(const Device& device, CommandHandle command, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
		: pDevice_(&device), command_(command), vertices_(vertices), indices_(indices)
	{
		CreateVertexBuffer();
		CreateIndexBuffer();
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
		return vertices_.size();
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
		return indices_.size();
	}

	bool ASMesh::LoadModelForAS(std::string modelPath, std::vector<ASVertex>& ASVertices, std::vector<uint32_t>& indices)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		string err, warn;

		bool success = loader.LoadASCIIFromFile(&model, &err, &warn, modelPath.c_str());
		if (!success) {
			cerr << "failed to load gltf\n";
			assert(0);
			return false;
		}

		ASVertices.clear();
		indices.clear();

		for (auto& mesh : model.meshes) {
			for (auto& primitive : mesh.primitives) {
				int posAccessorIndex = primitive.attributes["POSITION"];
				const tinygltf::Accessor& posAccessor = model.accessors[posAccessorIndex];
				const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
				tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
				float* positions = reinterpret_cast<float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);

				for (int i = 0; i < posAccessor.count; i++) {
					ASVertices.push_back(
						{
							(XMFLOAT3(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2])),
						});
				}

				int indicesAccessorIndex = primitive.indices;
				const tinygltf::Accessor& indicesAccessor = model.accessors[indicesAccessorIndex];
				const tinygltf::BufferView& indicesBufferView = model.bufferViews[indicesAccessor.bufferView];
				tinygltf::Buffer& indicesBuffer = model.buffers[indicesBufferView.buffer];
				unsigned short* index = reinterpret_cast<unsigned short*>(&indicesBuffer.data[indicesBufferView.byteOffset + indicesAccessor.byteOffset]);
				for (int i = 0; i < indicesAccessor.count; i++) {
					indices.push_back(index[i]);
				}
			}
		}
		return true;
	}

	HRESULT ASMesh::CreateVertexBuffer()
	{
		auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(ASVertices_.size() * sizeof(ASVertex));
		shared_ptr<Buffer> vertexUploadBuffer;
		vertexUploadBuffer = pDevice_->CreateBuffer(BufferType::Upload, sizeof(ASVertices_), ASVertices_.size());
		void* rawPtr = vertexUploadBuffer->Map();
		if (rawPtr) {
			ASVertex* pVertex = static_cast<ASVertex*>(rawPtr);
			memcpy(pVertex, ASVertices_.data(), sizeof(ASVertex) * ASVertices_.size());
			vertexUploadBuffer->Unmap();
		}

		vertexBuffer_ = pDevice_->CreateBuffer(BufferType::Default, sizeof(ASVertices_), ASVertices_.size());
		command_->CopyBuffer(vertexUploadBuffer, vertexBuffer_);
		command_->WaitCommand();
		vbView_.BufferLocation = vertexBuffer_->GetGPUAddress();
		vbView_.SizeInBytes = ASVertices_.size() * sizeof(ASVertex);
		vbView_.StrideInBytes = sizeof(ASVertex);

		return S_OK;
	}

	HRESULT ASMesh::CreateIndexBuffer()
	{
		auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(indices_.size() * sizeof(uint32_t));
		shared_ptr<Buffer> indexUploadBuffer;
		indexUploadBuffer = pDevice_->CreateBuffer(BufferType::Upload, sizeof(uint32_t), indices_.size());
		void* rawPtr = indexUploadBuffer->Map();
		if (rawPtr) {
			uint32_t* pIndex = static_cast<uint32_t*>(rawPtr);
			memcpy(pIndex, indices_.data(), sizeof(uint32_t) * indices_.size());
			indexUploadBuffer->Unmap();
		}

		indexBuffer_ = pDevice_->CreateBuffer(BufferType::Default, sizeof(uint32_t), indices_.size());
		command_->CopyBuffer(indexUploadBuffer, indexBuffer_);
		command_->WaitCommand();
		ibView_.BufferLocation = indexBuffer_->GetGPUAddress();
		ibView_.SizeInBytes = indices_.size() * sizeof(uint32_t);
		ibView_.Format = DXGI_FORMAT_R32_UINT;

		return S_OK;
	}

	ASMesh::ASMesh(const Device& device, CommandHandle command, std::string modelPath)
		: pDevice_(&device), command_(command)
	{
		LoadModelForAS(modelPath, ASVertices_, indices_);
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	ASMesh::ASMesh(const Device& device, CommandHandle command, const std::vector<ASVertex>& ASVertices, const std::vector<uint32_t>& indices)
		: pDevice_(&device), command_(command), ASVertices_(ASVertices), indices_(indices)
	{
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	BufferHandle ASMesh::GetVertexBuffer() const
	{
		return vertexBuffer_;
	}

	D3D12_VERTEX_BUFFER_VIEW ASMesh::GetVBView() const
	{
		return vbView_;
	}

	const D3D12_VERTEX_BUFFER_VIEW* ASMesh::GetVBViewPtr() const
	{
		return &vbView_;
	}

	UINT ASMesh::GetVertexCount() const
	{
		return ASVertices_.size();
	}

	BufferHandle ASMesh::GetIndexBuffer() const
	{
		return indexBuffer_;
	}

	D3D12_INDEX_BUFFER_VIEW ASMesh::GetIBView() const
	{
		return ibView_;
	}

	const D3D12_INDEX_BUFFER_VIEW* ASMesh::GetIBViewPtr() const
	{
		return &ibView_;
	}

	UINT ASMesh::GetNumIndices() const
	{
		return indices_.size();
	}
}