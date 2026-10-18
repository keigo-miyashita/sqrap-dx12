#pragma once

#include "pch.hpp"

namespace sqrp
{
	class ThreadPool
	{
	private:
		static thread_local int tlsWorkerIndex_;
		std::queue<std::function<void()>> taskQueue_;
		std::mutex tpMutex_;
		std::condition_variable tpCond_;
		std::condition_variable idleCond_;
		int activeCount_ = 0;
		bool isRequestedTermination_ = false;
		std::vector<std::thread> workers_;
		void ProcessTask(int index);
	public:
		static int CurrentWorkerIndex() { return tlsWorkerIndex_; }
		ThreadPool() = delete;
		ThreadPool(int numThreads);
		~ThreadPool();
		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;
		// Discard remaining task in queue and terminate
		void Abort();
		// Discard remaining task in queue and "not" terminate
		void CancelPendingTasks();
		template<typename F>
		void Push(F&& task)
		{
			{
				std::unique_lock<std::mutex> lk(tpMutex_);
				if (isRequestedTermination_) { return; }
				taskQueue_.push(std::forward<F>(task));
			}
			tpCond_.notify_all();
		}
		// Finish pending task and terminate
		void RequestTermination();
		// Finish pending task and "not" terminate
		void WaitUntilIdle();
	};
}