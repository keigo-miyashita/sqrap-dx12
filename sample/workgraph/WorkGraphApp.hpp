#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

struct Light
{
	DirectX::XMFLOAT4 pos;
	DirectX::XMFLOAT4 color;
};

struct Color
{
	DirectX::XMFLOAT4 color;
};

class WorkGraphApp : public sqrp::Application
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	//WorkGraphScene sampleScene_;

	sqrp::Device device_;
	sqrp::DXC dxc_;
	sqrp::CommandHandle command_;
	sqrp::SwapChainHandle swapChain_;
	sqrp::MeshHandle sphere_;
	// GUI
	sqrp::GUIHandle GUI_;

	// Scene Items
	sqrp::Camera camera_;
	Light light0_;
	sqrp::TransformMatrix sphere0_;

	// Resources
	sqrp::BufferHandle cameraBuffer_;
	sqrp::BufferHandle light0Buffer_;
	sqrp::BufferHandle sphere0Buffer_;
	Color diffuseColor_;

	// Shaders
	sqrp::ShaderHandle meshNode_;
	sqrp::ShaderHandle lambertPS_;

	// Descriptor
	sqrp::DescriptorManagerHandle sphere0DescManager_;

	// RootSignature
	sqrp::RootSignatureHandle sphere0RootSignature_;

	// StateObject
	sqrp::StateObjectHandle workGraphStateObject_;

	// Workgraph
	sqrp::WorkGraphHandle workGraph_;

public:
	WorkGraphApp(std::string windowName, unsigned int window_width = 1280, unsigned int window_height = 720);
	~WorkGraphApp() = default;
	virtual bool OnStart() override;
	virtual void OnUpdate() override;
	virtual void OnTerminate() override;
};