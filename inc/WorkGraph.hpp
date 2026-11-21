#pragma once

#include "pch.hpp"

#include "RootSignature.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class Buffer;
	class Device;
	class StateObject;
	class WorkGraph;

	// 複数のワークグラフで 1 つのバッキングメモリを共有する入れ物(2026-11-21、implicit-terrain の AMD 側で足した)。
	// ワークグラフごとに MaxSizeInBytes ずつ確保すると、グラフの多いアプリでは合計が VRAM を超えうる。同時に流さない
	// ワークグラフどうしなら 1 つのメモリを使い回せる(D3D12 Work Graphs の仕様: 別のワークグラフが使ったバッキングメモリは、
	// 前の使用が終わってから D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE を立てて使う。MaxSizeInBytes より大きい範囲を渡してよい)。
	//   - 大きさは、共有するワークグラフの MaxSizeInBytes のいちばん大きい値。初めて使うとき(Acquire)に確保する
	//     (その後に大きいワークグラフを足したら確保し直す。古いメモリは使い終わりが分からないので、この入れ物が消えるまで持つ)
	//   - Command::SetProgram は、前にこの入れ物を使ったワークグラフと違うワークグラフを張るときだけ、全体の UAV バリアを置いて
	//     前の実行の終わりを待ち、INITIALIZE を立てる。同じワークグラフを続けて張るときは何も足さない(共有しないときと同じ動き)
	//   - **同じコマンドリストの中で、間にバリアを置かずに続けて流すワークグラフどうしは共有しないこと**
	//     (DispatchGraph どうしには暗黙のバリアが無く、重なって走りうる)
	//   - メインスレッドから使う(ロックしない)
	class WorkGraphBackingPool
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device* device_ = nullptr;
		std::wstring name_;
		UINT64 requiredSize_ = 0;      // 共有するワークグラフの MaxSizeInBytes のいちばん大きい値
		UINT64 sumOfMaxSizes_ = 0;     // 共有しなかったときの合計(ログ用)
		UINT numUsers_ = 0;
		ComPtr<ID3D12Resource> buffer_ = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE range_ = {};
		std::vector<ComPtr<ID3D12Resource>> retired_;
		const WorkGraph* lastUser_ = nullptr;

	public:
		WorkGraphBackingPool(const Device& device, std::wstring name);
		WorkGraphBackingPool(const WorkGraphBackingPool&) = delete;
		WorkGraphBackingPool& operator=(const WorkGraphBackingPool&) = delete;

		// ワークグラフを 1 つ足す(WorkGraph のコンストラクタが呼ぶ)
		void Register(const D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS& memReqs);
		// 確保していなければ確保し、範囲を返す
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE Acquire();
		// 前にこの入れ物を使ったワークグラフと違えば true を返し、使い手を user に替える(Command::SetProgram が呼ぶ)
		bool SwitchUser(const WorkGraph* user);
		const ComPtr<ID3D12Resource>& GetBuffer() const;
		UINT64 GetRequiredSize() const;
		UINT64 GetSumOfMaxSizes() const;
		UINT GetNumUsers() const;
	};

	class WorkGraph
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		using IndexInWorkGraphDesc = UINT;

		std::wstring name_;
		ComPtr<ID3D12WorkGraphProperties1> workGraphProp_ = nullptr;
		D3D12_PROGRAM_IDENTIFIER workGraphProgramID_ = {};
		D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS memReqs_ = {};
		// D3D12_GPU_VIRTUAL_ADDRESSとsizeInBytesをもつ構造体
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE backingMemoryAddressRange_ = {};
		ComPtr<ID3D12Resource> backingMemory_ = nullptr;
		// 共有のバッキングメモリ(2026-11-21)。null なら今までどおり自分で MaxSizeInBytes を確保する
		WorkGraphBackingPoolHandle backingPool_;
		BufferHandle localRootSigBuffer_;
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE localRootArgumentsTableAddress_ = {};
		UINT workGraphIndex_ = 0;
		UINT numEntryPoints_ = 0;
		UINT numNodes_ = 0;

	public:
		static UINT AlignForLocalSigBuffer(UINT size);
		static UINT CopyMem(void* dest, const void* data, UINT size);
		D3D12_SET_PROGRAM_DESC MakeSetProgramDesc(bool initialize = true)const;
		WorkGraph(const Device& device, std::wstring name, const StateObjectHandle& stateObject,
			std::vector<std::vector<LocalRootArguments>> computeLocalRootArguments,
			std::vector<std::vector<LocalRootArguments>> graphicsLocalRootArguments,
			UINT maxInputRecords = 0, UINT maxNodeInputs = 0,
			const WorkGraphBackingPoolHandle& backingPool = nullptr);
		WorkGraph(const WorkGraph&) = delete;
		WorkGraph& operator=(const WorkGraph&) = delete;

		// 共有のときは共有のメモリ(まだ確保していなければここで確保する)
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE GetBackingMemoryAddressRange() const;
		const ComPtr<ID3D12Resource>& GetBackingMemory() const;
		const WorkGraphBackingPoolHandle& GetBackingPool() const;
		const D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS& GetMemoryRequirements() const;
		D3D12_PROGRAM_IDENTIFIER GetProgramID() const;
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetLocalRootArgumentsTable() const;
	};
}
