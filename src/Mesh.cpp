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

	bool Mesh::LoadModel(string modelPath, vector<Vertex>& vertices, vector<uint32_t>& indices)
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

	void MeshBase::CreateVertexBuffer()
	{
		shared_ptr<Buffer> vertexUploadBuffer;
		vertexUploadBuffer = pDevice_->CreateBuffer(name_ + L"_upload_Vertex", BufferType::Upload, sizeof(Vertex), vertices_.size());
		void* rawPtr = vertexUploadBuffer->Map();
		if (rawPtr) {
			Vertex* pVertex = static_cast<Vertex*>(rawPtr);
			memcpy(pVertex, vertices_.data(), sizeof(Vertex) * vertices_.size());
			vertexUploadBuffer->Unmap();
		}

		BufferType type = isWritable_ ? BufferType::Unordered : BufferType::Default;
		vertexBuffer_ = pDevice_->CreateBuffer(name_ + L"_Vertex", type, sizeof(Vertex), vertices_.size());

		command_->CopyBuffer(vertexUploadBuffer, vertexUploadBuffer->GetInitialState(), vertexBuffer_, vertexBuffer_->GetInitialState());
		command_->WaitCommand();
		vbView_.BufferLocation = vertexBuffer_->GetGPUAddress();
		vbView_.SizeInBytes = vertices_.size() * sizeof(Vertex);
		vbView_.StrideInBytes = sizeof(Vertex);
	}

	void MeshBase::CreateIndexBuffer()
	{
		shared_ptr<Buffer> indexUploadBuffer;
		indexUploadBuffer = pDevice_->CreateBuffer(name_ + L"_upload_Index", BufferType::Upload, sizeof(uint32_t), indices_.size());
		void* rawPtr = indexUploadBuffer->Map();
		if (rawPtr) {
			uint32_t* pIndex = static_cast<uint32_t*>(rawPtr);
			memcpy(pIndex, indices_.data(), sizeof(uint32_t) * indices_.size());
			indexUploadBuffer->Unmap();
		}

		BufferType type = isWritable_ ? BufferType::Unordered : BufferType::Default;
		indexBuffer_ = pDevice_->CreateBuffer(name_ + L"_Index", type, sizeof(uint32_t), indices_.size());

		command_->CopyBuffer(indexUploadBuffer, indexUploadBuffer->GetInitialState(), indexBuffer_, indexBuffer_->GetInitialState());
		command_->WaitCommand();
		ibView_.BufferLocation = indexBuffer_->GetGPUAddress();
		ibView_.SizeInBytes = indices_.size() * sizeof(uint32_t);
		ibView_.Format = DXGI_FORMAT_R32_UINT;
	}

	MeshBase::MeshBase(const Device& device, CommandHandle command, wstring name)
		: pDevice_(&device), command_(command), name_(name)
	{
	}

	Mesh::Mesh(const Device& device, CommandHandle command, string modelPath)
		: MeshBase(device, command)
	{
		filesystem::path fullPath = modelPath;
		string name = fullPath.stem().string();
		name_ = StringToWString(name);

		LoadModel(modelPath, vertices_, indices_);
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	Mesh::Mesh(const Device& device, wstring name, CommandHandle command, const vector<Vertex>& vertices, const vector<uint32_t>& indices)
		: MeshBase(device, command, name)
	{
		vertices_ = vertices;
		indices_ = indices;
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	Mesh::Mesh(const Device& device, wstring name, CommandHandle command, UINT verticesNum, UINT indicesNum)
		: MeshBase(device, command, name)
	{
		isWritable_ = true;
		vertices_.resize(verticesNum);
		indices_.resize(indicesNum);
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	void MeshBase::UpdateVBView(UINT numVertices)
	{
		vbView_.SizeInBytes = numVertices * sizeof(Vertex);
	}

	void MeshBase::UpdateIBView(UINT numIndices)
	{
		ibView_.SizeInBytes = numIndices * sizeof(uint32_t);
	}

	BufferHandle MeshBase::GetVertexBuffer() const
	{
		return vertexBuffer_;
	}

	D3D12_VERTEX_BUFFER_VIEW MeshBase::GetVBView() const
	{
		return vbView_;
	}

	const D3D12_VERTEX_BUFFER_VIEW* MeshBase::GetVBViewPtr() const
	{
		return &vbView_;
	}

	UINT MeshBase::GetVertexCount() const
	{
		return vertices_.size();
	}

	BufferHandle MeshBase::GetIndexBuffer() const
	{
		return indexBuffer_;
	}

	D3D12_INDEX_BUFFER_VIEW MeshBase::GetIBView() const
	{
		return ibView_;
	}

	const D3D12_INDEX_BUFFER_VIEW* MeshBase::GetIBViewPtr() const
	{
		return &ibView_;
	}

	UINT MeshBase::GetNumIndices() const
	{
		return indices_.size();
	}

	// ===================================================================
	// GLTFMesh
	// ===================================================================

	GLTFMesh::GLTFMesh(const Device& device, CommandHandle command, string modelPath)
		: MeshBase(device, command)
	{
		filesystem::path fullPath = modelPath;
		string name = fullPath.stem().string();
		name_ = StringToWString(name);

		LoadModel(modelPath);
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	bool GLTFMesh::LoadModel(string modelPath)
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

		uint32_t vertexOffset = 0;
		uint32_t indexOffset = 0;
		int meshIndex = 0;
		meshNum_ = static_cast<int>(model.meshes.size());

		for (auto& mesh : model.meshes) {
			int primitiveIndex = 0;
			for (auto& primitive : mesh.primitives) {
				int posAccessorIndex = primitive.attributes["POSITION"];
				const tinygltf::Accessor& posAccessor = model.accessors[posAccessorIndex];
				const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
				tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
				float* positions = reinterpret_cast<float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);

				float* normals = nullptr;
				bool hasNormal = primitive.attributes.contains("NORMAL");
				if (hasNormal) {
					int normalAccessorIndex = primitive.attributes["NORMAL"];
					const tinygltf::Accessor& normalAccessor = model.accessors[normalAccessorIndex];
					const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor.bufferView];
					tinygltf::Buffer& normalBuffer = model.buffers[normalBufferView.buffer];
					normals = reinterpret_cast<float*>(&normalBuffer.data[normalBufferView.byteOffset + normalAccessor.byteOffset]);
				}

				float* tangents = nullptr;
				bool hasTangent = primitive.attributes.contains("TANGENT");
				if (hasTangent) {
					int tangentAccessorIndex = primitive.attributes["TANGENT"];
					const tinygltf::Accessor& tangentAccessor = model.accessors[tangentAccessorIndex];
					const tinygltf::BufferView& tangentBufferView = model.bufferViews[tangentAccessor.bufferView];
					tinygltf::Buffer& tangentBuffer = model.buffers[tangentBufferView.buffer];
					tangents = reinterpret_cast<float*>(&tangentBuffer.data[tangentBufferView.byteOffset + tangentAccessor.byteOffset]);
				}

				float* uvs = nullptr;
				bool hasUV = primitive.attributes.contains("TEXCOORD_0");
				if (hasUV) {
					int uvAccessorIndex = primitive.attributes["TEXCOORD_0"];
					const tinygltf::Accessor& uvAccessor = model.accessors[uvAccessorIndex];
					const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor.bufferView];
					tinygltf::Buffer& uvBuffer = model.buffers[uvBufferView.buffer];
					uvs = reinterpret_cast<float*>(&uvBuffer.data[uvBufferView.byteOffset + uvAccessor.byteOffset]);
				}

				for (int i = 0; i < posAccessor.count; i++) {
					Vertex v{};
					v.position = XMFLOAT4(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2], 1.0f);
					if (hasNormal)  v.normal  = XMFLOAT4(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2], 1.0f);
					if (hasTangent) v.tangent = XMFLOAT4(tangents[i * 4], tangents[i * 4 + 1], tangents[i * 4 + 2], tangents[i * 4 + 3]);
					if (hasUV)      v.uv      = XMFLOAT2(uvs[i * 2], uvs[i * 2 + 1]);
					vertices_.push_back(v);
				}

				int indicesAccessorIndex = primitive.indices;
				const tinygltf::Accessor& indicesAccessor = model.accessors[indicesAccessorIndex];
				const tinygltf::BufferView& indicesBufferView = model.bufferViews[indicesAccessor.bufferView];
				tinygltf::Buffer& indicesBuffer = model.buffers[indicesBufferView.buffer];
				unsigned short* index = reinterpret_cast<unsigned short*>(&indicesBuffer.data[indicesBufferView.byteOffset + indicesAccessor.byteOffset]);
				for (int i = 0; i < indicesAccessor.count; i++) {
					indices_.push_back(index[i]);
				}

				vertexRanges_[make_pair(meshIndex, primitiveIndex)] = { vertexOffset, static_cast<uint32_t>(vertices_.size() - vertexOffset) };
				indexRanges_[make_pair(meshIndex, primitiveIndex)] = { indexOffset, static_cast<uint32_t>(indices_.size() - indexOffset) };
				materialIndices_[make_pair(meshIndex, primitiveIndex)] = primitive.material;

				vertexOffset = static_cast<uint32_t>(vertices_.size());
				indexOffset = static_cast<uint32_t>(indices_.size());
				primitiveIndex++;
			}
			primitiveNumPerMesh_.push_back(primitiveIndex);
			meshIndex++;
		}

		for (const auto& node : model.nodes) {
			XMMATRIX modelMat = XMMatrixIdentity();
			if (node.matrix.size() == 16) {
				// glTF matrix is column-major; copied sequentially into a row-major
				// XMFLOAT4X4 it becomes the row-vector form used by this renderer.
				XMFLOAT4X4 m;
				float* dst = &m._11;
				for (int i = 0; i < 16; i++) dst[i] = static_cast<float>(node.matrix[i]);
				modelMat = XMLoadFloat4x4(&m);
			}
			else {
				XMVECTOR t = XMVectorZero();
				XMVECTOR q = XMQuaternionIdentity();
				XMVECTOR s = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
				if (node.translation.size() == 3)
					t = XMVectorSet((float)node.translation[0], (float)node.translation[1], (float)node.translation[2], 0.0f);
				if (node.rotation.size() == 4)
					q = XMVectorSet((float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3]);
				if (node.scale.size() == 3)
					s = XMVectorSet((float)node.scale[0], (float)node.scale[1], (float)node.scale[2], 0.0f);
				modelMat = XMMatrixScalingFromVector(s) * XMMatrixRotationQuaternion(q) * XMMatrixTranslationFromVector(t);
			}

			if (node.mesh != -1) {
				SubMeshInfo info;
				info.meshIndex = node.mesh;
				info.model = modelMat;
				info.invTransModel = XMMatrixTranspose(XMMatrixInverse(nullptr, modelMat));
				subMeshInfos_.push_back(info);
			}
		}
		return true;
	}

	int GLTFMesh::GetMeshNum() const
	{
		return meshNum_;
	}

	int GLTFMesh::GetPrimitiveNumPerMesh(int meshIndex) const
	{
		return primitiveNumPerMesh_[meshIndex];
	}

	GLTFMesh::MeshRange GLTFMesh::GetVertexRange(int meshIndex, int primitiveIndex) const
	{
		return vertexRanges_.at(make_pair(meshIndex, primitiveIndex));
	}

	GLTFMesh::MeshRange GLTFMesh::GetIndexRange(int meshIndex, int primitiveIndex) const
	{
		return indexRanges_.at(make_pair(meshIndex, primitiveIndex));
	}

	int GLTFMesh::GetMaterialIndex(int meshIndex, int primitiveIndex) const
	{
		return materialIndices_.at(make_pair(meshIndex, primitiveIndex));
	}

	const vector<GLTFMesh::SubMeshInfo>& GLTFMesh::GetSubMeshInfos() const
	{
		return subMeshInfos_;
	}

	int GLTFMesh::GetNumIndices(int meshIndex, int primitiveIndex) const
	{
		return static_cast<int>(indexRanges_.at(make_pair(meshIndex, primitiveIndex)).count);
	}

	XMMATRIX GLTFMesh::GetGltfModelMat() const
	{
		if (!subMeshInfos_.empty()) return subMeshInfos_[0].model;
		return XMMatrixIdentity();
	}

	bool ASMesh::LoadModelForAS(string modelPath, vector<ASVertex>& ASVertices, vector<uint32_t>& indices)
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

	void ASMesh::CreateVertexBuffer()
	{
		shared_ptr<Buffer> vertexUploadBuffer;
		vertexUploadBuffer = pDevice_->CreateBuffer(L"_upload_Vertex", BufferType::Upload, sizeof(ASVertex), ASVertices_.size());
		void* rawPtr = vertexUploadBuffer->Map();
		if (rawPtr) {
			ASVertex* pVertex = static_cast<ASVertex*>(rawPtr);
			memcpy(pVertex, ASVertices_.data(), sizeof(ASVertex) * ASVertices_.size());
			vertexUploadBuffer->Unmap();
		}

		vertexBuffer_ = pDevice_->CreateBuffer(L"_Vertex", BufferType::Default, sizeof(ASVertex), ASVertices_.size());

		command_->CopyBuffer(vertexUploadBuffer, vertexUploadBuffer->GetInitialState(), vertexBuffer_, vertexBuffer_->GetInitialState());
		command_->WaitCommand();
		vbView_.BufferLocation = vertexBuffer_->GetGPUAddress();
		vbView_.SizeInBytes = ASVertices_.size() * sizeof(ASVertex);
		vbView_.StrideInBytes = sizeof(ASVertex);
	}

	void ASMesh::CreateIndexBuffer()
	{
		shared_ptr<Buffer> indexUploadBuffer;
		indexUploadBuffer = pDevice_->CreateBuffer(L"_upload_Index", BufferType::Upload, sizeof(uint32_t), indices_.size());
		void* rawPtr = indexUploadBuffer->Map();
		if (rawPtr) {
			uint32_t* pIndex = static_cast<uint32_t*>(rawPtr);
			memcpy(pIndex, indices_.data(), sizeof(uint32_t) * indices_.size());
			indexUploadBuffer->Unmap();
		}

		indexBuffer_ = pDevice_->CreateBuffer(L"_Index", BufferType::Default, sizeof(uint32_t), indices_.size());

		command_->CopyBuffer(indexUploadBuffer, indexUploadBuffer->GetInitialState(), indexBuffer_, indexBuffer_->GetInitialState());
		command_->WaitCommand();
		ibView_.BufferLocation = indexBuffer_->GetGPUAddress();
		ibView_.SizeInBytes = indices_.size() * sizeof(uint32_t);
		ibView_.Format = DXGI_FORMAT_R32_UINT;
	}

	ASMesh::ASMesh(const Device& device, CommandHandle command, string modelPath)
		: pDevice_(&device), command_(command)
	{
		filesystem::path fullPath = modelPath;
		string name = fullPath.stem().string();
		name_ = StringToWString(name);
		LoadModelForAS(modelPath, ASVertices_, indices_);
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	ASMesh::ASMesh(const Device& device, wstring name, CommandHandle command, const vector<ASVertex>& ASVertices, const vector<uint32_t>& indices)
		: pDevice_(&device), command_(command), ASVertices_(ASVertices), indices_(indices), name_(name)
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