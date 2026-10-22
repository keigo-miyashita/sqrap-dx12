#include "MeshShaderApp.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;
using namespace sqrp;

namespace {
// GUI で選ぶ色は「画面に出す値」（sRGB で符号化された値）なので、GPU の計算に使う前に線形に直す。
// バックバッファの RTV は sRGB なので、出口で1回だけ符号化される。アルファは変換しない
float SrgbToLinear(float c)
{
	return (c <= 0.04045f) ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}
Color SrgbToLinear(const Color& c)
{
	return { XMFLOAT4(SrgbToLinear(c.color.x), SrgbToLinear(c.color.y), SrgbToLinear(c.color.z), c.color.w) };
}
// ルート定数は記録時にこのポインタから読まれるので、線形に直した diffuseColor_ をここに置く
Color gpuDiffuseColor;
}

MeshShaderApp::MeshShaderApp(std::string windowName, unsigned int window_width, unsigned int window_height) : Application(windowName, window_width, window_height)
{

}

bool MeshShaderApp::OnStart()
{
	device_.Init(L"NVIDIA");

	dxc_.Init();

	gui_ = device_.CreateGui(GetWindowHWND());

	command_ = device_.CreateCommand(L"General");

	SIZE size = { GetWindowWidth(), GetWindowHeight() };
	swapChain_ = device_.CreateSwapChain(L"Main", GetWindowHWND(), size);

	// Objects data
	string modelPath = string(MODEL_DIR) + "Suzanne.gltf";
	sphere_ = device_.CreateMesh(L"Suzanne", command_, modelPath);

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
	cameraBuffer_ = device_.CreateBuffer(L"camera", BufferType::Upload, Buffer::AlignForConstantBuffer(sizeof(CameraMatrix)), 1);
	cameraBuffer_->Write(CameraMatrix{ camera_.GetView(), camera_.GetProj() });

	light0Buffer_ = device_.CreateBuffer(L"light0", BufferType::Upload, Buffer::AlignForConstantBuffer(sizeof(Light)), 1);
	light0Buffer_->Write(light0_);

	sphere0Buffer_ = device_.CreateBuffer(L"sphere0", BufferType::Upload, Buffer::AlignForConstantBuffer(sizeof(TransformMatrix)), 1);
	sphere0Buffer_->Write(sphere0_);

	diffuseColor_ = { {1.0f, 0.0f, 0.0f, 1.0f} };
	gpuDiffuseColor = SrgbToLinear(diffuseColor_);

	// Shaders
	wstring shaderPath = wstring(SHADER_DIR) + L"MeshShader.hlsl";
	simpleMS_ = dxc_.CreateShader(ShaderType::Mesh, shaderPath, L"MSmain");
	lambertPS_ = dxc_.CreateShader(ShaderType::Pixel, shaderPath, L"PSmain");

	// Descriptor Manager
	sphere0DescManager_ = device_.CreateDescriptorManager(
		L"sphere0",
		HeapType::Resource,
		{
			{ .resource_ = cameraBuffer_,				.type_ = ViewType::CBV, .numReg_ = 0},
			{ .resource_ = light0Buffer_,				.type_ = ViewType::CBV, .numReg_ = 1},
			{ .resource_ = sphere0Buffer_,				.type_ = ViewType::CBV, .numReg_ = 2},
			{ .resource_ = sphere_->GetVertexBuffer(),	.type_ = ViewType::SRV, .numReg_ = 0 },
			{ .resource_ = sphere_->GetIndexBuffer(),	.type_ = ViewType::SRV, .numReg_ = 1 }
		}
	);
	// RootSignature
	sphere0RootSignature_ = device_.CreateRootSignature(
		L"sphere0",
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
		{
			{.rootParamType_ = RootParamType::DescTable,	.rootParamDesc_ = sphere0DescManager_},
			{.rootParamType_ = RootParamType::Constant,	.rootParamDesc_ = DirectRootParamDesc{.numReg_ = 3, .numConstant_ = 4, .constantPtr_ = (void*)(&gpuDiffuseColor)}},
		}
	);

	MeshDesc meshDesc = {};
	meshDesc.rootSignature_ = sphere0RootSignature_;
	meshDesc.MS_ = simpleMS_;
	meshDesc.PS_ = lambertPS_;
	lambert_ = device_.CreateMeshPipeline(L"lamber", meshDesc);

	return true;
};

void MeshShaderApp::OnUpdate()
{
	command_->BeginRender(swapChain_);

	camera_.Update();
	cameraBuffer_->Write(CameraMatrix{ camera_.GetView(), camera_.GetProj() });
	gpuDiffuseColor = SrgbToLinear(diffuseColor_);

	UINT num_dispatch_thread = sphere_->GetNumIndices() / 3 / 64 + 1;

	command_->GetStableCommandList()->SetPipelineState(lambert_->GetPipelineState().Get());
	command_->SetGraphicsResource(sphere0RootSignature_);
	command_->GetStableCommandList()->DispatchMesh(num_dispatch_thread, 1, 1);

	// GUI
	{
		gui_->BeginCommand();
		// Add GUI 
		ImGui::Text("Color");
		ImGui::SameLine();
		ImGui::ColorEdit4("##SurfaceColor", (float*)&diffuseColor_);
		gui_->EndCommand();
		command_->DrawGui(gui_, swapChain_);
	}

	command_->EndRender(swapChain_);
};

void MeshShaderApp::OnTerminate()
{
	command_->WaitCommand();   // Present 完了まで待つ (WaitSignal だけでは Present 前のシグナルしか待てない)
};