#include "MeshShaderApp.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;
using namespace sqrp;

MeshShaderApp::MeshShaderApp(std::string windowName, unsigned int window_width, unsigned int window_height) : Application(windowName, window_width, window_height)
{

}

bool MeshShaderApp::OnStart()
{
	device_.Init(L"NVIDIA");

	dxc_.Init();

	GUI_ = device_.CreateGUI(GetWindowHWND());

	command_ = device_.CreateCommand();

	SIZE size = { GetWindowWidth(), GetWindowHeight() };
	swapChain_ = device_.CreateSwapChain(command_, GetWindowHWND(), size);

	// Objects data
	string modelPath = string(MODEL_DIR) + "Suzanne.gltf";
	sphere_ = device_.CreateMesh(command_, modelPath);

	// Scene Items
	// Camera
	camera_.Init((float)GetWindowWidth() / (float)GetWindowHeight(), XMFLOAT3(0.0f, 0.0f, -5.0f));

	// Light
	light0_.pos = XMFLOAT4(10.0f, 10.0f, -5.0f, 1.0f);
	light0_.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// Sphere0
	XMMATRIX modelMat = XMMatrixMultiply(XMMatrixRotationY(XMConvertToRadians(180)), XMMatrixIdentity());
	sphere0_.model = modelMat;
	sphere0_.invTransModel = XMMatrixTranspose(XMMatrixInverse(nullptr, sphere0_.model));

	// Resources
	cameraBuffer_ = device_.CreateBuffer(BufferType::Upload, Buffer::AlignForConstantBuffer(sizeof(CameraMatrix)), 1);
	cameraBuffer_->Write(CameraMatrix{ camera_.GetView(), camera_.GetProj() });

	light0Buffer_ = device_.CreateBuffer(BufferType::Upload, Buffer::AlignForConstantBuffer(sizeof(Light)), 1);
	light0Buffer_->Write(light0_);

	sphere0Buffer_ = device_.CreateBuffer(BufferType::Upload, Buffer::AlignForConstantBuffer(sizeof(TransformMatrix)), 1);
	sphere0Buffer_->Write(sphere0_);

	diffuseColor_ = { {1.0f, 0.0f, 0.0f, 1.0f} };

	// Shaders
	wstring shaderPath = wstring(SHADER_DIR) + L"MeshShader.hlsl";
	simpleMS_ = dxc_.CreateShader(ShaderType::Mesh, shaderPath, L"MSmain");
	lambertPS_ = dxc_.CreateShader(ShaderType::Pixel, shaderPath, L"PSmain");

	// Descriptor Manager
	sphere0DescManager_ = device_.CreateDescriptorManager(
		HeapType::Resource,
		{
			{ cameraBuffer_,				ViewType::CBV, 0},
			{ light0Buffer_,				ViewType::CBV, 1},
			{ sphere0Buffer_,				ViewType::CBV, 2},
			{ sphere_->GetVertexBuffer(),	ViewType::SRV, 0 },
			{ sphere_->GetIndexBuffer(),	ViewType::SRV, 1 }
		}
	);
	// RootSignature
	sphere0RootSignature_ = device_.CreateRootSignature(
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
		{
			{RootParamType::DescTable,	sphere0DescManager_},
			{RootParamType::Constant,	DirectRootParamDesc{3, 4, (void*)(&diffuseColor_)}},
		}
	);

	MeshDesc meshDesc = {};
	meshDesc.rootSignature_ = sphere0RootSignature_;
	meshDesc.MS_ = simpleMS_;
	meshDesc.PS_ = lambertPS_;
	lambert_ = device_.CreateMeshPipeline(meshDesc);

	return true;
};

void MeshShaderApp::OnUpdate()
{
	command_->BeginRender(swapChain_);

	camera_.Update();
	cameraBuffer_->Write(CameraMatrix{ camera_.GetView(), camera_.GetProj() });

	UINT num_dispatch_thread = sphere_->GetNumIndices() / 3 / 64 + 1;

	command_->GetStableCommandList()->SetPipelineState(lambert_->GetPipelineState().Get());
	command_->SetGraphicsResource(sphere0RootSignature_);
	command_->GetStableCommandList()->DispatchMesh(num_dispatch_thread, 1, 1);

	// GUI
	{
		GUI_->BeginCommand();
		// Add GUI 
		ImGui::Text("Color");
		ImGui::SameLine();
		ImGui::ColorEdit4("##SurfaceColor", (float*)&diffuseColor_);
		GUI_->EndCommand();
		command_->DrawGUI(GUI_);
	}

	command_->EndRender(swapChain_);
};

void MeshShaderApp::OnTerminate()
{

};