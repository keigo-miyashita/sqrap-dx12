#include "Command.hpp"

#include "DescriptorManager.hpp"
#include "Device.hpp"
#include "Fence.hpp"
#include "Indirect.hpp"
#include "Gui.hpp"
#include "Mesh.hpp"
#include "Pipeline.hpp"
#include "Resource.hpp"
#include "RootSignature.hpp"
#include "SwapChain.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	void Command::CreateCommandList()
	{
		for (UINT i = 0; i < frameCount_; i++)
		{
			ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
			HRESULT result = pDevice_->GetDevice()->CreateCommandAllocator(commandType_, IID_PPV_ARGS(commandAllocator.ReleaseAndGetAddressOf()));
			if (FAILED(result)) {
				throw runtime_error("[Command::CreateCommandList] CreateCommandAllocator failed\nHRESULT = " + to_string(result));
			}
			wstring cmdAllocName = L"CommandAllocator";
			commandAllocator->SetName((cmdAllocName + name_ + to_wstring(i)).c_str());
			commandAllocators_.push_back(commandAllocator);
		}
		HRESULT result = pDevice_->GetDevice()->CreateCommandList(0, commandType_, commandAllocators_[0].Get(), nullptr, IID_PPV_ARGS(commandList_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[Command::CreateCommandList] CreateCommandList failed\nHRESULT = " + to_string(result));
		}
		wstring cmdListName = L"CommandList";
		commandList_->SetName((cmdListName + name_).c_str());
	}

	void Command::InitializeStableCommandList()
	{
		HRESULT result = commandList_->QueryInterface(IID_PPV_ARGS(stableCommandList_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[Command::InitializeStableCommandList] QueryInterface for StableCommandList failed\nHRESULT = " + to_string(result));
		}
		stableCommandList_->SetName((L"StableCommandList" + name_).c_str());
	}

	void Command::InitializeLatestCommandList()
	{
		HRESULT result = commandList_->QueryInterface(IID_PPV_ARGS(latestCommandList_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[Command::InitializeLatestCommandList] QueryInterface for LatestCommandList failed\nHRESULT = " + to_string(result));
		}
		latestCommandList_->SetName((L"LatestCommandList" + name_).c_str());
	}

	Command::Command(const Device& device, wstring name, D3D12_COMMAND_LIST_TYPE commandType, UINT frameCount)
		: pDevice_(&device), name_(name), commandType_(commandType), frameCount_(frameCount)
	{
		switch (commandType_) {
			case D3D12_COMMAND_LIST_TYPE_DIRECT:
				queueType_ = QueueType::Graphics;
				break;
			case D3D12_COMMAND_LIST_TYPE_COMPUTE:
				queueType_ = QueueType::Compute;
				break;
			default:
				throw runtime_error("[Command::Command] unsupported command list type");
		}

		if (frameCount_ == 0) {
			throw runtime_error("[Command::Command] frameCount must be greater than 0");
		}

		CreateCommandList();

		InitializeStableCommandList();

		InitializeLatestCommandList();

		fenceValues_.resize(frameCount_, 0);
	}

	void Command::BeginRender(const SwapChainHandle& swapChain)
	{
		if (queueType_ != QueueType::Graphics) {
			throw runtime_error("[Command::BeginRender] BeginRender can only be called on a graphics command list");
		}

		auto bbIdx = swapChain->GetSwapChain()->GetCurrentBackBufferIndex();

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChain->GetCurrentBackBuffer()->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		commandList_->ResourceBarrier(1, &barrier);

		auto rtvHandle = swapChain->GetRtvHandle(bbIdx);
		auto dsvHandle = swapChain->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
		commandList_->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

		// Clear depth buffer
		commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// Clear display
		// RTV は sRGB なので、クリアの色も線形の値で渡す（0.214 は画面上の 0.5 の灰色）
		float clearColor[] = { 0.214f, 0.214f, 0.214f, 1.0f };
		commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		// Set viewport and scissors
		auto viewPort = swapChain->GetViewPort();
		auto scissorRect = swapChain->GetRect();
		commandList_->RSSetViewports(1, &(viewPort));
		commandList_->RSSetScissorRects(1, &(scissorRect));
	}

	void Command::EndRender(const SwapChainHandle& swapChain)
	{
		if (queueType_ != QueueType::Graphics) {
			throw runtime_error("[Command::EndRender] EndRender can only be called on a graphics command list");
		}

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChain->GetCurrentBackBuffer()->GetResource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		commandList_->ResourceBarrier(1, &barrier);

		Submit();

		swapChain->GetSwapChain()->Present(1, 0);
	}

	void Command::AddDrawIndexed(const MeshBaseHandle& mesh, UINT numInstances)
	{
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList_->IASetVertexBuffers(0, 1, mesh->GetVBViewPtr());
		commandList_->IASetIndexBuffer(mesh->GetIBViewPtr());
		commandList_->DrawIndexedInstanced(mesh->GetNumIndices(), 1, 0, 0, 0);
	}

	void Command::AddDrawIndexedLine(const MeshHandle& mesh, UINT numInstances)
	{
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		commandList_->IASetVertexBuffers(0, 1, mesh->GetVBViewPtr());
		commandList_->IASetIndexBuffer(mesh->GetIBViewPtr());
		commandList_->DrawIndexedInstanced(mesh->GetNumIndices(), 1, 0, 0, 0);
	}

	void Command::Barrier(UINT numBarriers, D3D12_RESOURCE_BARRIER* pBarriers)
	{
		commandList_->ResourceBarrier(numBarriers, pBarriers);
	}

	void Command::Barrier(vector<D3D12_RESOURCE_BARRIER> barriers)
	{
		commandList_->ResourceBarrier(barriers.size(), barriers.data());
	}

	void Command::CopyBuffer(const ResourceHandle& srcResource, D3D12_RESOURCE_STATES srcCurrentState, const ResourceHandle& destResource, D3D12_RESOURCE_STATES destCurrentState)
	{
		vector<CD3DX12_RESOURCE_BARRIER> rscBarriers;
		if (srcCurrentState != D3D12_RESOURCE_STATE_COPY_SOURCE) {
			auto srcBarrier = CD3DX12_RESOURCE_BARRIER::Transition(srcResource->GetResource().Get(), srcCurrentState, D3D12_RESOURCE_STATE_COPY_SOURCE);
			rscBarriers.push_back(srcBarrier);
		}
		if (destCurrentState != D3D12_RESOURCE_STATE_COPY_DEST) {
			auto destBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destResource->GetResource().Get(), destCurrentState, D3D12_RESOURCE_STATE_COPY_DEST);
			rscBarriers.push_back(destBarrier);
		}

		if (rscBarriers.size() != 0) {
			commandList_->ResourceBarrier(rscBarriers.size(), rscBarriers.data());
		}
		commandList_->CopyResource(destResource->GetResource().Get(), srcResource->GetResource().Get());

		rscBarriers.clear();
		if (srcCurrentState != D3D12_RESOURCE_STATE_COPY_SOURCE) {
			auto srcbarrier = CD3DX12_RESOURCE_BARRIER::Transition(srcResource->GetResource().Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, srcCurrentState);
			rscBarriers.push_back(srcbarrier);
		}
		if (destCurrentState != D3D12_RESOURCE_STATE_COPY_DEST) {
			auto destbarrier = CD3DX12_RESOURCE_BARRIER::Transition(destResource->GetResource().Get(), D3D12_RESOURCE_STATE_COPY_DEST, destCurrentState);
			rscBarriers.push_back(destbarrier);
		}
		if (rscBarriers.size() != 0) {
			commandList_->ResourceBarrier(rscBarriers.size(), rscBarriers.data());
		}
	}

	void Command::CopyBufferRegion(const BufferHandle& srcBuffer, UINT64 srcOffset, D3D12_RESOURCE_STATES srcCurrentState, const BufferHandle& destBuffer, UINT64 destOffset, D3D12_RESOURCE_STATES destCurrentState, UINT64 numBytes)
	{
		vector<CD3DX12_RESOURCE_BARRIER> rscBarriers;
		if (srcCurrentState != D3D12_RESOURCE_STATE_COPY_SOURCE) {
			auto srcBarrier = CD3DX12_RESOURCE_BARRIER::Transition(srcBuffer->GetResource().Get(), srcCurrentState, D3D12_RESOURCE_STATE_COPY_SOURCE);
			rscBarriers.push_back(srcBarrier);
		}
		if (destCurrentState != D3D12_RESOURCE_STATE_COPY_DEST) {
			auto destBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer->GetResource().Get(), destCurrentState, D3D12_RESOURCE_STATE_COPY_DEST);
			rscBarriers.push_back(destBarrier);
		}

		if (rscBarriers.size() != 0) {
			commandList_->ResourceBarrier(rscBarriers.size(), rscBarriers.data());
		}
		commandList_->CopyBufferRegion(destBuffer->GetResource().Get(), destOffset, srcBuffer->GetResource().Get(), srcOffset, numBytes);

		rscBarriers.clear();
		if (srcCurrentState != D3D12_RESOURCE_STATE_COPY_SOURCE) {
			auto srcbarrier = CD3DX12_RESOURCE_BARRIER::Transition(srcBuffer->GetResource().Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, srcCurrentState);
			rscBarriers.push_back(srcbarrier);
		}
		if (destCurrentState != D3D12_RESOURCE_STATE_COPY_DEST) {
			auto destbarrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer->GetResource().Get(), D3D12_RESOURCE_STATE_COPY_DEST, destCurrentState);
			rscBarriers.push_back(destbarrier);
		}
		if (rscBarriers.size() != 0) {
			commandList_->ResourceBarrier(rscBarriers.size(), rscBarriers.data());
		}
	}

	void Command::Dispatch(UINT threadX, UINT threadY, UINT threadZ)
	{
		commandList_->Dispatch(threadX, threadY, threadZ);
	}

	void Command::DispatchRays(const RaytracingHandle& rayTracing)
	{
		D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = rayTracing->GetDispatchRayDesc();
		stableCommandList_->DispatchRays(&dispatchRaysDesc);
	}

	void Command::DrawIndirect(const MeshHandle& mesh, const IndirectHandle& indirect, const BufferHandle& buffer, UINT maxCommandNum)
	{
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList_->IASetVertexBuffers(0, 1, mesh->GetVBViewPtr());
		commandList_->IASetIndexBuffer(mesh->GetIBViewPtr());
		commandList_->ExecuteIndirect(indirect->GetCommandSignature().Get(), maxCommandNum, buffer->GetResource().Get(), 0, nullptr, 0);
	}

	void Command::DrawGui(const GuiHandle& gui, const SwapChainHandle& swapChain)
	{
		// ImGui の色は画面に出す値そのものなので、符号化しない UNORM の RTV に描く。
		// ImGui の PSO の形式（Gui の ImGui_ImplDX12_Init に渡した UNORM）とも一致する
		const UINT bbIdx = swapChain->GetSwapChain()->GetCurrentBackBufferIndex();
		auto guiRtvHandle = swapChain->GetUnormRtvHandle(bbIdx);
		commandList_->OMSetRenderTargets(1, &guiRtvHandle, false, nullptr);

		commandList_->SetDescriptorHeaps(1, gui->GetImGuiDescHeap().GetAddressOf());
		gui->Draw(*this);

		// GUI の後にも場面を描けるよう、sRGB の RTV と深度に戻しておく
		auto rtvHandle = swapChain->GetRtvHandle(bbIdx);
		auto dsvHandle = swapChain->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
		commandList_->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
	}

	void Command::ExecuteIndirect(const IndirectHandle& indirect)
	{
		if (indirect->GetCounterBuffer() != nullptr) {
			commandList_->ExecuteIndirect(
				indirect->GetCommandSignature().Get(),
				indirect->GetMaxCommandCount(),
				indirect->GetArgumentBuffer()->GetResource().Get(),
				0,
				indirect->GetCounterBuffer()->GetResource().Get(),
				indirect->GetCounterBufferOffset()
			);
		}
		else {
			commandList_->ExecuteIndirect(
				indirect->GetCommandSignature().Get(),
				indirect->GetMaxCommandCount(),
				indirect->GetArgumentBuffer()->GetResource().Get(),
				0,
				nullptr,
				0
			);
		}
	}

	void Command::Close()
	{
		commandList_->Close();
	}

	void Command::SetMesh(const MeshBaseHandle& mesh)
	{
		commandList_->IASetVertexBuffers(0, 1, mesh->GetVBViewPtr());
		commandList_->IASetIndexBuffer(mesh->GetIBViewPtr());
	}

	void Command::SetComputeResource(const RootSignatureHandle& computeRootSig)
	{
		SetComputeRootSig(computeRootSig);

		vector<ID3D12DescriptorHeap*> heaps;
		for (const auto& rootParam_ : computeRootSig->GetRootParametersVec()) {
			if (rootParam_.rootParamType_ == RootParamType::DescTable) {
				DescriptorManagerHandle descManager = get<DescriptorManagerHandle>(rootParam_.rootParamDesc_);
				auto heap = descManager->GetDescriptorHeap().Get();
				if (find(heaps.begin(), heaps.end(), heap) == heaps.end()) {
					heaps.push_back(heap);
				}
			}
		}
		if (!heaps.empty()) {
			commandList_->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
		}

		int rootParamIndex = 0;
		for (const auto& rootParam_ : computeRootSig->GetRootParametersVec()) {
			if (rootParam_.rootParamType_ == RootParamType::DescTable) {
				DescriptorManagerHandle descManager = get<DescriptorManagerHandle>(rootParam_.rootParamDesc_);
				SetComputeRootDescriptorTable(rootParamIndex, descManager);
			}
			else if (rootParam_.rootParamType_ == RootParamType::CBV) {
				DirectRootParamDesc directDesc = get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				// NOTE : to handle CBV
			}
			else if (rootParam_.rootParamType_ == RootParamType::SRV) {
				DirectRootParamDesc directDesc = get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				// NOTE : to handle SRV
			}
			else if (rootParam_.rootParamType_ == RootParamType::UAV) {
				DirectRootParamDesc directDesc = get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				// NOTE : to handle UAV
			}
			else if (rootParam_.rootParamType_ == RootParamType::Constant) {
				DirectRootParamDesc directDesc = get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				//SetComputeRoot32BitConstants(rootParamIndex, /*pointer*/ nullptr);
				commandList_->SetComputeRoot32BitConstants(rootParamIndex, directDesc.numConstant_, directDesc.constantPtr_, 0);
			}
			rootParamIndex++;
		}
	}

	void Command::SetComputeRoot32BitConstants(UINT rootParamIndex, const ConstantsHandle& constant)
	{
		commandList_->SetComputeRoot32BitConstants(rootParamIndex, constant->GetNumConstants(), constant->GetConstants(), 0);
	}

	void Command::SetComputeRootDescriptorTable(UINT rootParamIndex, const DescriptorManagerHandle& descManager)
	{
		commandList_->SetComputeRootDescriptorTable(rootParamIndex, descManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
	}

	void Command::SetComputeRootSig(const RootSignatureHandle& computeRootSig)
	{
		commandList_->SetComputeRootSignature(computeRootSig->GetRootSignature().Get());
	}

	void Command::SetDescriptorHeap(const DescriptorManagerHandle& descManager)
	{
		commandList_->SetDescriptorHeaps(1, descManager->GetDescriptorHeap().GetAddressOf());
	}

	void Command::SetGraphicsResource(const RootSignatureHandle& graphicsRootSig)
	{
		SetGraphicsRootSig(graphicsRootSig);

		vector<ID3D12DescriptorHeap*> heaps;
		for (const auto& rootParam_ : graphicsRootSig->GetRootParametersVec()) {
			if (rootParam_.rootParamType_ == RootParamType::DescTable) {
				DescriptorManagerHandle descManager = get<DescriptorManagerHandle>(rootParam_.rootParamDesc_);
				auto heap = descManager->GetDescriptorHeap().Get();
				if (find(heaps.begin(), heaps.end(), heap) == heaps.end()) {
					heaps.push_back(heap);
				}
			}
		}
		if (!heaps.empty()) {
			commandList_->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
		}

		int rootParamIndex = 0;
		for (const auto& rootParam_ : graphicsRootSig->GetRootParametersVec()) {
			if (rootParam_.rootParamType_ == RootParamType::DescTable) {
				DescriptorManagerHandle descManager = get<DescriptorManagerHandle>(rootParam_.rootParamDesc_);
				SetGraphicsRootDescriptorTable(rootParamIndex, descManager);
			}
			else if (rootParam_.rootParamType_ == RootParamType::CBV) {
				DirectRootParamDesc directDesc = get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				// NOTE : to handle CBV
			}
			else if (rootParam_.rootParamType_ == RootParamType::SRV) {
				DirectRootParamDesc directDesc = get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				// NOTE : to handle SRV
			}
			else if (rootParam_.rootParamType_ == RootParamType::UAV) {
				DirectRootParamDesc directDesc = get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				// NOTE : to handle UAV
			}
			else if (rootParam_.rootParamType_ == RootParamType::Constant) {
				DirectRootParamDesc directDesc = get<DirectRootParamDesc>(rootParam_.rootParamDesc_);
				//SetGraphicsRoot32BitConstants(rootParamIndex, /*pointer*/ nullptr);
				commandList_->SetGraphicsRoot32BitConstants(rootParamIndex, directDesc.numConstant_, directDesc.constantPtr_, 0);
			}
			rootParamIndex++;
		}
	}

	void Command::SetGraphicsRoot32BitConstants(UINT rootParamIndex, const ConstantsHandle& constant)
	{
		commandList_->SetGraphicsRoot32BitConstants(rootParamIndex, constant->GetNumConstants(), constant->GetConstants(), 0);
	}

	void Command::SetGraphicsRootDescriptorTable(UINT rootParamIndex, const DescriptorManagerHandle& descManager)
	{
		commandList_->SetGraphicsRootDescriptorTable(rootParamIndex, descManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
	}

	void Command::SetGraphicsRootSig(const RootSignatureHandle& graphicsRootSig)
	{
		commandList_->SetGraphicsRootSignature(graphicsRootSig->GetRootSignature().Get());
	}

	void Command::SetRaytracingResource(const RootSignatureHandle& rayTracingRootSig)
	{
		SetComputeResource(rayTracingRootSig);
	}

	void Command::SetPipeline(const GraphicsPipelineHandle& graphicsPipeline)
	{
		commandList_->SetPipelineState(graphicsPipeline->GetPipelineState().Get());
	}

	void Command::SetPipeline(const ComputePipelineHandle& computePipeline)
	{
		commandList_->SetPipelineState(computePipeline->GetPipelineState().Get());
	}

	void Command::SetPipeline(const MeshPipelineHandle& meshPipeline)
	{
		commandList_->SetPipelineState(meshPipeline->GetPipelineState().Get());
	}

	void Command::SetRaytracingState(const StateObjectHandle& stateObject)
	{
		stableCommandList_->SetPipelineState1(stateObject->GetStateObject().Get());
	}

	SyncPoint Command::Submit()
	{
		Close();

		ID3D12CommandList* cmdLists[] = { commandList_.Get() };
		pDevice_->GetCommandQueue(queueType_)->ExecuteCommandLists(1, cmdLists);

		FenceHandle fence = pDevice_->GetFence(queueType_);
		const UINT64 fenceVal = fence->Signal();
		fenceValues_[frameIndex_] = fenceVal;

		frameIndex_ = (frameIndex_ + 1) % frameCount_;
		fence->WaitSignal(fenceValues_[frameIndex_]);

		pDevice_->ReleaseCompleted();

		commandAllocators_[frameIndex_]->Reset();
		commandList_->Reset(commandAllocators_[frameIndex_].Get(), nullptr);

		return { queueType_, fenceVal };
	}

	void Command::WaitCommand()
	{
		pDevice_->WaitOnCPU(Submit());
	}

	void Command::InitDataToBuffer(const BufferHandle& buffer, void* pData, UINT strideSize, UINT numElement)
	{
		BufferHandle uploadBuffer = pDevice_->CreateBuffer(L"Upload", BufferType::Upload, strideSize, numElement);

		uploadBuffer->Upload(pData, strideSize, numElement);

		this->CopyBufferRegion(uploadBuffer, 0, uploadBuffer->GetInitialState(), buffer, 0, buffer->GetInitialState(), strideSize * numElement);

		this->WaitCommand();
	}

	void Command::InitDataToTexture(const TextureHandle& texture, void* pData, UINT strideSize, UINT width, UINT height)
	{
		auto desc = texture->GetResource()->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
		UINT numRows;
		UINT64 rowSizeInBytes, totalBytes;
		pDevice_->GetDevice()->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalBytes);

		BufferHandle uploadBuffer = pDevice_->CreateBuffer(L"TextureUpload", BufferType::Upload, 1, static_cast<UINT>(totalBytes));

		void* pMapped = uploadBuffer->Map();
		if (pMapped) {
			uint8_t* dst = static_cast<uint8_t*>(pMapped);
			const uint8_t* src = static_cast<const uint8_t*>(pData);
			UINT srcRowPitch = strideSize * width;
			for (UINT row = 0; row < numRows; ++row) {
				memcpy(dst + row * footprint.Footprint.RowPitch, src + row * srcRowPitch, srcRowPitch);
			}
			uploadBuffer->Unmap();
		}

		auto toDestBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			texture->GetResource().Get(), texture->GetInitialState(), D3D12_RESOURCE_STATE_COPY_DEST);
		commandList_->ResourceBarrier(1, &toDestBarrier);

		D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
		srcLoc.pResource = uploadBuffer->GetResource().Get();
		srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLoc.PlacedFootprint = footprint;

		D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
		dstLoc.pResource = texture->GetResource().Get();
		dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstLoc.SubresourceIndex = 0;

		commandList_->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

		auto toInitBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			texture->GetResource().Get(), D3D12_RESOURCE_STATE_COPY_DEST, texture->GetInitialState());
		commandList_->ResourceBarrier(1, &toInitBarrier);

		WaitCommand();
	}

	const ComPtr<ID3D12CommandAllocator>& Command::GetCurrentCommandAllocator() const
	{
		return commandAllocators_[frameIndex_];
	}

	const ComPtr<ID3D12GraphicsCommandList>& Command::GetCommandList() const
	{
		return commandList_;
	}

	const ComPtr<StableCommandList>& Command::GetStableCommandList() const
	{
		return stableCommandList_;
	}

	const ComPtr<LatestCommandList>& Command::GetLatestCommandList() const
	{
		return latestCommandList_;
	}

	QueueType Command::GetQueueType() const
	{
		return queueType_;
	}

	UINT Command::GetFrameIndex() const
	{
		return frameIndex_;
	}

	UINT Command::GetFrameCount() const
	{
		return frameCount_;
	}
}