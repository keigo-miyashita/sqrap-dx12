#include "GLTFLoader.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tiny_gltf.h>

namespace sqrp
{
	GLTFModel LoadGLTF(const std::string& modelPath)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		string err, warn;

		bool success = loader.LoadASCIIFromFile(&model, &err, &warn, modelPath.c_str());
		if (!success) {
			throw runtime_error("[LoadGLTF] LoadASCIIFromFile failed\n" + err);
		}

		GLTFModel gltfModel;
		vector<pair<UINT, UINT>> subMeshRanges;

		uint32_t vertexOffset = 0;
		uint32_t indexOffset = 0;

		for (auto& mesh : model.meshes) {
			const UINT subMeshIndex = static_cast<UINT>(gltfModel.mesh.subMeshes.size());
			for (auto& primitive : mesh.primitives) {
				auto posItr = primitive.attributes.find("POSITION");
				if (posItr == primitive.attributes.end()) {
					throw runtime_error("[LoadGLTF] POSITION attribute not found in primitive");
				}
				int posAccessorIndex = posItr->second;
				const tinygltf::Accessor& posAccessor = model.accessors[posAccessorIndex];
				const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
				tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
				float* positions = reinterpret_cast<float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);

				float* normals = nullptr;
				auto normalItr = primitive.attributes.find("NORMAL");
				if (normalItr != primitive.attributes.end()) {
					int normalAccessorIndex = normalItr->second;
					const tinygltf::Accessor& normalAccessor = model.accessors[normalAccessorIndex];
					const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor.bufferView];
					tinygltf::Buffer& normalBuffer = model.buffers[normalBufferView.buffer];
					normals = reinterpret_cast<float*>(&normalBuffer.data[normalBufferView.byteOffset + normalAccessor.byteOffset]);
				}

				float* tangents = nullptr;
				auto tangentItr = primitive.attributes.find("TANGENT");
				if (tangentItr != primitive.attributes.end()) {
					int tangentAccessorIndex = tangentItr->second;
					const tinygltf::Accessor& tangentAccessor = model.accessors[tangentAccessorIndex];
					const tinygltf::BufferView& tangentBufferView = model.bufferViews[tangentAccessor.bufferView];
					tinygltf::Buffer& tangentBuffer = model.buffers[tangentBufferView.buffer];
					tangents = reinterpret_cast<float*>(&tangentBuffer.data[tangentBufferView.byteOffset + tangentAccessor.byteOffset]);
				}

				float* uvs = nullptr;
				auto uvItr = primitive.attributes.find("TEXCOORD_0");
				if (uvItr != primitive.attributes.end()) {
					int uvAccessorIndex = uvItr->second;
					const tinygltf::Accessor& uvAccessor = model.accessors[uvAccessorIndex];
					const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor.bufferView];
					tinygltf::Buffer& uvBuffer = model.buffers[uvBufferView.buffer];
					uvs = reinterpret_cast<float*>(&uvBuffer.data[uvBufferView.byteOffset + uvAccessor.byteOffset]);
				}

				for (int i = 0; i < posAccessor.count; i++) {
					Vertex v{};
					v.position = XMFLOAT4(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2], 1.0f);
					if (normals)  v.normal = XMFLOAT4(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2], 1.0f);
					if (tangents) v.tangent = XMFLOAT4(tangents[i * 4], tangents[i * 4 + 1], tangents[i * 4 + 2], tangents[i * 4 + 3]);
					if (uvs)      v.uv = XMFLOAT2(uvs[i * 2], uvs[i * 2 + 1]);
					gltfModel.mesh.vertices.push_back(v);
				}

				int indicesAccessorIndex = primitive.indices;
				const tinygltf::Accessor& indicesAccessor = model.accessors[indicesAccessorIndex];
				const tinygltf::BufferView& indicesBufferView = model.bufferViews[indicesAccessor.bufferView];
				tinygltf::Buffer& indicesBuffer = model.buffers[indicesBufferView.buffer];
				unsigned short* index = reinterpret_cast<unsigned short*>(&indicesBuffer.data[indicesBufferView.byteOffset + indicesAccessor.byteOffset]);
				for (int i = 0; i < indicesAccessor.count; i++) {
					gltfModel.mesh.indices.push_back(index[i]);
				}

				SubMesh subMesh;
				subMesh.baseVertex = static_cast<int>(vertexOffset);
				subMesh.indexOffset = indexOffset;
				subMesh.indexCount = static_cast<UINT>(indicesAccessor.count);
				subMesh.materialIndex = primitive.material;
				gltfModel.mesh.subMeshes.push_back(subMesh);

				vertexOffset = static_cast<uint32_t>(gltfModel.mesh.vertices.size());
				indexOffset = static_cast<uint32_t>(gltfModel.mesh.indices.size());
			}
			subMeshRanges.push_back({ subMeshIndex, static_cast<UINT>(gltfModel.mesh.subMeshes.size()) - subMeshIndex });
		}

		auto localMatrix = [](const tinygltf::Node& node) -> XMMATRIX {
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
			return modelMat;
		};

		function<void(int, XMMATRIX)> traverseNode = [&](int nodeIndex, XMMATRIX parentMat) {
			const tinygltf::Node& node = model.nodes[nodeIndex];
			XMMATRIX globalMat = localMatrix(node) * parentMat;
			if (node.mesh != -1) {
				GLTFNode gltfNode;
				gltfNode.meshIndex = node.mesh;
				gltfNode.subMeshIndex = subMeshRanges[node.mesh].first;
				gltfNode.subMeshCount = subMeshRanges[node.mesh].second;
				gltfNode.model = globalMat;
				gltfNode.invTransModel = XMMatrixTranspose(XMMatrixInverse(nullptr, globalMat));
				gltfModel.nodes.push_back(gltfNode);
			}
			for (int childIndex : node.children) {
				traverseNode(childIndex, globalMat);
			}
		};

		vector<int> rootNodes;
		if (!model.scenes.empty())
		{
			const int sceneIndex = model.defaultScene >= 0 ? model.defaultScene : 0;
			rootNodes = model.scenes[sceneIndex].nodes;
		}
		else {
			vector<bool> isChild(model.nodes.size(), false);
			for (const auto& node : model.nodes) {
				for (int childIndex : node.children) {
					isChild[childIndex] = true;
				}
			}
			for (int i = 0; i < static_cast<int>(model.nodes.size()); ++i) {
				if (!isChild[i]) {
					rootNodes.push_back(i);
				}
			}
		}

		for (int rootNode : rootNodes) {
			traverseNode(rootNode, XMMatrixIdentity());
		}

		return gltfModel;
	}
}