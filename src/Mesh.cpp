#include <common.hpp>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tiny_gltf.h>

bool Mesh::LoadModel(string modelPath)
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

	vertices_.clear();
	indices_.clear();

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
				vertices_.push_back(
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
				indices_.push_back(index[i]);
			}
		}
	}
	return true;
}

HRESULT Mesh::CreateVertexBuffer(Command& command, Fence& fence)
{
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices_.size() * sizeof(Vertex));
	Buffer vertexUploadBuffer;
	vertexUploadBuffer.InitAsUpload(pDevice_, sizeof(Vertex), vertices_.size());
	//if (FAILED(pDevice_->GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(vertexBuffer_.ReleaseAndGetAddressOf())))) {
	//	return S_FALSE;
	//}
	/*Vertex* vertMap = nullptr;
	if (FAILED(vertexBuffer_->Map(0, nullptr, (void**)&vertMap))) {
		return S_FALSE;
	}
	std::copy(std::begin(vertices_), std::end(vertices_), vertMap);
	vertexBuffer_->Unmap(0, nullptr);*/
	void* rawPtr = vertexUploadBuffer.Map();
	if (rawPtr) {
		Vertex* pVertex = static_cast<Vertex*>(rawPtr);
		memcpy(pVertex, vertices_.data(), sizeof(Vertex) * vertices_.size());
		vertexUploadBuffer.Unmap();
	}

	vertexBuffer_.Init(pDevice_, sizeof(Vertex), vertices_.size());
	command.CopyBuffer(vertexUploadBuffer, vertexBuffer_);
	fence.WaitCommand(command);
	//vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vbView_.BufferLocation = vertexBuffer_.GetGPUAddress();
	vbView_.SizeInBytes = vertices_.size() * sizeof(Vertex);
	vbView_.StrideInBytes = sizeof(Vertex);

	return S_OK;
}

HRESULT Mesh::CreateIndexBuffer(Command& command, Fence& fence)
{
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(indices_.size() * sizeof(uint32_t));
	Buffer indexUploadBuffer;
	indexUploadBuffer.InitAsUpload(pDevice_, sizeof(uint32_t), indices_.size());
	/*if (FAILED(pDevice_->GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(indexBuffer_.ReleaseAndGetAddressOf())))) {
		return S_FALSE;
	}*/
	/*uint32_t* idxMap = nullptr;
	if (FAILED(indexBuffer_->Map(0, nullptr, (void**)&idxMap))) {
		return S_FALSE;
	}
	std::copy(std::begin(indices_), std::end(indices_), idxMap);
	indexBuffer_->Unmap(0, nullptr);*/
	void* rawPtr = indexUploadBuffer.Map();
	if (rawPtr) {
		uint32_t* pIndex = static_cast<uint32_t*>(rawPtr);
		memcpy(pIndex, indices_.data(), sizeof(uint32_t) * indices_.size());
		indexUploadBuffer.Unmap();
	}

	indexBuffer_.Init(pDevice_, sizeof(uint32_t), indices_.size());
	command.CopyBuffer(indexUploadBuffer, indexBuffer_);
	fence.WaitCommand(command);
	//ibView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
	ibView_.BufferLocation = indexBuffer_.GetGPUAddress();
	ibView_.SizeInBytes = indices_.size() * sizeof(uint32_t);
	ibView_.Format = DXGI_FORMAT_R32_UINT;

	return S_OK;
}

Mesh::Mesh()
{

}

bool Mesh::Init(Device* pDevice, Command& command_, Fence& fence, std::string modelPath)
{
	pDevice_ = pDevice;
	if (pDevice_ == nullptr) {
		cerr << "Mesh class doesn't have any pointer" << endl;
		return false;
	}
	if (!LoadModel(modelPath)) {
		return false;
	}

	if (FAILED(CreateVertexBuffer(command_, fence))) {
		return false;
	}

	if (FAILED(CreateIndexBuffer(command_, fence))) {
		return false;
	}


	return true;
}

bool Mesh::Init(Device* pDevice, Command& command_, Fence& fence_, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
	pDevice_ = pDevice;
	vertices_ = vertices;
	indices_ = indices;
	
	if (FAILED(CreateVertexBuffer(command_, fence_))) {
		return false;
	}

	if (FAILED(CreateIndexBuffer(command_, fence_))) {
		return false;
	}

	return true;
}

const Buffer& Mesh::GetVertexBuffer() const
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

const Buffer& Mesh::GetIndexBuffer() const
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

bool ASMesh::LoadModel(std::string modelPath)
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

	ASVertices_.clear();
	indices_.clear();

	for (auto& mesh : model.meshes) {
		for (auto& primitive : mesh.primitives) {
			int posAccessorIndex = primitive.attributes["POSITION"];
			const tinygltf::Accessor& posAccessor = model.accessors[posAccessorIndex];
			const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
			tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
			float* positions = reinterpret_cast<float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);

			for (int i = 0; i < posAccessor.count; i++) {
				ASVertices_.push_back(
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
				indices_.push_back(index[i]);
			}
		}
	}
	return true;
}

HRESULT ASMesh::CreateVertexBuffer(Command& command, Fence& fence)
{
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(ASVertices_.size() * sizeof(ASVertex));
	Buffer vertexUploadBuffer;
	vertexUploadBuffer.InitAsUpload(pDevice_, sizeof(ASVertices_), ASVertices_.size());
	/*if (FAILED(pDevice_->GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(vertexBuffer_.ReleaseAndGetAddressOf())))) {
		return S_FALSE;
	}*/
	/*ASVertex* vertMap = nullptr;
	if (FAILED(vertexBuffer_->Map(0, nullptr, (void**)&vertMap))) {
		return S_FALSE;
	}
	std::copy(std::begin(ASVertices_), std::end(ASVertices_), vertMap);
	vertexBuffer_->Unmap(0, nullptr);*/
	void* rawPtr = vertexUploadBuffer.Map();
	if (rawPtr) {
		ASVertex* pVertex = static_cast<ASVertex*>(rawPtr);
		memcpy(pVertex, ASVertices_.data(), sizeof(ASVertex) * ASVertices_.size());
		vertexUploadBuffer.Unmap();
	}

	vertexBuffer_.Init(pDevice_, sizeof(ASVertices_), ASVertices_.size());
	command.CopyBuffer(vertexUploadBuffer, vertexBuffer_);
	fence.WaitCommand(command);
	//vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vbView_.BufferLocation = vertexBuffer_.GetGPUAddress();
	vbView_.SizeInBytes = ASVertices_.size() * sizeof(ASVertex);
	vbView_.StrideInBytes = sizeof(ASVertex);

	return S_OK;
}

ASMesh::ASMesh()
{

}

UINT ASMesh::GetVertexCount() const
{
	return ASVertices_.size();
}