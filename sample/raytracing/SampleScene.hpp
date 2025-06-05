#pragma once

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

	// Mesh
	ASMeshHandle sphereASMesh_;
	ASMeshHandle teapotASMesh_;

	// Scene Items
	Camera camera_;
	Light light0_;

	// AccelerationStructure
	BLASHandle sphereBLAS_;
	BLASHandle teapotBLAS_;
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
	DescriptorManagerHandle sphere0DescManager_;

	// RootSignature
	RootSignatureHandle sphere0RootSignature_;

	// Resource Set
	std::shared_ptr<ResourceSet> sphere0ResourceSet_;

	// Pipeline
	StateObjectHandle raytracingStates_;

	// RayTracing
	RayTracingHandle rayTracing_;

	void BeginRender();
	void EndRender();

public:
	SampleScene();
	~SampleScene() = default;
	bool Init(const Application& app);
	void Render();
};