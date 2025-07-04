#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

struct CameraMatrix
{
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX proj;
	DirectX::XMMATRIX invViewProj;
	DirectX::XMMATRIX invView;
	DirectX::XMFLOAT4 cameraPosition;
};

struct Light
{
	DirectX::XMFLOAT4 pos;
	DirectX::XMFLOAT4 color;
};

struct Object
{
	DirectX::XMMATRIX model;
	DirectX::XMMATRIX invTransModel;
};

struct Color
{
	DirectX::XMFLOAT4 color;
};

class SampleScene
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	Device device_;
	DXC dxc_;
	CommandHandle command_;
	SwapChainHandle swapChain_;
	// GUI
	GUIHandle GUI_;

	// Mesh
	ASMeshHandle suzanneASMesh_;
	MeshHandle suzanneMesh_;

	// Scene Items
	Camera camera_;
	Light light0_;

	// AccelerationStructure
	BLASHandle suzanneBLAS_;
	TLASHandle sceneTLAS_;

	// Resources
	BufferHandle cameraBuffer_;
	BufferHandle light0Buffer_;
	TextureHandle outputTexture_;

	// Shaders
	ShaderHandle rayGen_;
	ShaderHandle closestHit_;
	ShaderHandle miss_;

	// Descriptor
	DescriptorManagerHandle suzanneDescManager_;

	// RootSignature
	RootSignatureHandle suzanneRootSignature_;

	// Resource Set
	ResourceSetHandle suzanneResourceSet_;

	// Pipeline
	StateObjectHandle rayTracingStates_;

	// RayTracing
	RayTracingHandle rayTracing_;

	ConstantsHandle ColorConstants_;
	Color diffuseColor_;

	void BeginRender();
	void EndRender();

public:
	SampleScene();
	~SampleScene() = default;
	bool Init(const Application& app);
	void Render();
};