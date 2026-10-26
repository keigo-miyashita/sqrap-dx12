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
	sqrp::MeshHandle sphere_;
	// GUI
	sqrp::GuiHandle gui_;

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
	sqrp::ShaderHandle simpleVS_;
	sqrp::ShaderHandle lambertPS_;

	// Descriptor
	sqrp::DescriptorManagerHandle sphere0DescManager_;

	// RootSignature
	sqrp::RootSignatureHandle sphere0RootSignature_;

	// Pipeline
	sqrp::GraphicsPipelineHandle lambert_;

public:
	SampleApplication(std::string windowName, unsigned int window_width = 1280, unsigned int window_height = 720);
	~SampleApplication() = default;
	virtual bool OnStart() override;
	virtual void OnUpdate() override;
	virtual void OnTerminate() override;
	virtual void OnResize(UINT width, UINT height) override;
};