#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

struct RayTracigCameraMatrix
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

class SampleApplication : public sqrp::Application
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	//SampleScene sampleScene_;
	// Device はハンドルより先に宣言する（メンバは宣言と逆の順に壊れるので、Device が最後に壊れる）
	sqrp::Device device_;
	sqrp::Dxc dxc_;
	sqrp::CommandHandle command_;
	sqrp::SwapChainHandle swapChain_;
	// GUI
	sqrp::GuiHandle gui_;

	// Mesh
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
	// Shared by the root signature and the descriptors recreated on resize
	sqrp::DescriptorLayout suzanneLayout_{ sqrp::DescriptorLayoutType::Resource, { {sqrp::ViewType::CBV, 0}, {sqrp::ViewType::CBV, 1}, {sqrp::ViewType::SRV, 0}, {sqrp::ViewType::SRV, 1}, {sqrp::ViewType::SRV, 2}, {sqrp::ViewType::UAV, 0} } };
	sqrp::DescriptorManagerHandle suzanneDescManager_;

	// RootSignature
	sqrp::RootSignatureHandle suzanneRootSignature_;

	// Resource Set

	// Pipeline
	sqrp::StateObjectHandle rayTracingStates_;

	// RayTracing
	sqrp::RaytracingHandle rayTracing_;

	sqrp::ConstantsHandle ColorConstants_;
	Color diffuseColor_;

	// Create the window-sized output texture and its descriptors
	void CreateOutputResources();

public:
	SampleApplication(std::string windowName, unsigned int window_width = 1280, unsigned int window_height = 720);
	~SampleApplication() = default;
	virtual bool OnStart() override;
	virtual void OnUpdate() override;
	virtual void OnTerminate() override;
	virtual void OnResize(UINT width, UINT height) override;
};