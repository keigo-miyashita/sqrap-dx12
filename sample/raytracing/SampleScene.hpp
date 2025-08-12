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

struct Color
{
	DirectX::XMFLOAT4 color;
};

class SampleScene
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	sqrp::Device device_;
	sqrp::DXC dxc_;
	sqrp::CommandHandle command_;
	sqrp::SwapChainHandle swapChain_;
	// GUI
	sqrp::GUIHandle GUI_;

	// Mesh
	sqrp::ASMeshHandle suzanneASMesh_;
	sqrp::MeshHandle suzanneMesh_;

	// Scene Items
	sqrp::Camera camera_;
	Light light0_;

	// AccelerationStructure
	sqrp::BLASHandle suzanneBLAS_;
	sqrp::TLASHandle sceneTLAS_;

	// Resources
	sqrp::BufferHandle cameraBuffer_;
	sqrp::BufferHandle light0Buffer_;
	sqrp::TextureHandle outputTexture_;

	// Shaders
	sqrp::ShaderHandle rayGen_;
	sqrp::ShaderHandle closestHit_;
	sqrp::ShaderHandle miss_;

	// Descriptor
	sqrp::DescriptorManagerHandle suzanneDescManager_;

	// RootSignature
	sqrp::RootSignatureHandle suzanneRootSignature_;

	// Resource Set
	sqrp::ResourceSetHandle suzanneResourceSet_;

	// Pipeline
	sqrp::StateObjectHandle rayTracingStates_;

	// RayTracing
	sqrp::RayTracingHandle rayTracing_;

	sqrp::ConstantsHandle ColorConstants_;
	Color diffuseColor_;

	void BeginRender();
	void EndRender();

public:
	SampleScene();
	~SampleScene() = default;
	bool Init(const sqrp::Application& app);
	void Render();
};