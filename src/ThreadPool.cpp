#include "ThreadPool.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	thread_local int ThreadPool::tlsWorkerIndex_ = -1;

	void ThreadPool::ProcessTask(int index)
	{
		tlsWorkerIndex_ = index;
		// std::thread starts fucntion at constructor immediately, 
		// so we need infinite loop to keep thread alive 
		// and check task quque has tasks to process
		while (true)
		{
			function<void()> task;
			{
				unique_lock<mutex> lk(tpMutex_);
				//											false || false -> wait, no task, wait occuring task
				//											false || true  -> start terminate, no task
				//											true  || false -> wait, remain task
				//											true  || true  -> start terminate, "but" remain task
				tpCond_.wait(lk, [this]() { return !(taskQueue_.empty()) || isRequestedTermination_; });
				// If empty is true, there are no tasks
				if (taskQueue_.empty()) { return; }
				// If empty is false, there is task
				task = std::move(taskQueue_.front());
				taskQueue_.pop();
				++(activeCount_);
			}
			try {
				task();
			}
			catch (const exception& e)
			{
				// Don't throw exception from thread, because it will terminate the program
				cerr << "ThreadPool::ProcessTask() exception: " << e.what() << endl;
			}
			catch (...)
			{
				cerr << "ThreadPool::ProcessTask() unknown exception" << endl;
			}
			{
				unique_lock<mutex> lk(tpMutex_);
				--(activeCount_);
				if (taskQueue_.empty() && activeCount_ == 0)
				{
					idleCond_.notify_all();
				}
			}
		}
	}

	ThreadPool::ThreadPool(int numThreads)
	{
		workers_.reserve(numThreads);
		for (int i = 0; i < numThreads; i++) {
			workers_.emplace_back(thread([this, i]() {ProcessTask(i); }));
		}
	}

	ThreadPool::~ThreadPool()
	{
		RequestTermination();
		// Thread destructor is called here
		// ThreadPool has sync primitive 
		// so we need to desctruct thread before sync primitive
		for (auto& worker : workers_) {
			if (worker.joinable()) {
				worker.join();
			}
		}
	}

	void ThreadPool::Abort()
	{
		queue<function<void()>> emptyQueue;
		bool becomeIdle = false;
		{
			unique_lock<mutex> lk(tpMutex_);
			isRequestedTermination_ = true;
			taskQueue_.swap(emptyQueue);
			becomeIdle = (taskQueue_.empty() && activeCount_ == 0);
		}
		tpCond_.notify_all();
		if (becomeIdle) {
			idleCond_.notify_all();
		}
	}

	void ThreadPool::CancelPendingTasks()
	{
		queue<function<void()>> emptyQueue;
		bool becomeIdle = false;
		{
			unique_lock<mutex> lk(tpMutex_);
			taskQueue_.swap(emptyQueue);
			becomeIdle = (taskQueue_.empty() && activeCount_ == 0);
		}
		if (becomeIdle) {
			idleCond_.notify_all();
		}
	}

	void ThreadPool::RequestTermination()
	{
		{
			unique_lock<mutex> lk(tpMutex_);
			isRequestedTermination_ = true;
		}
		tpCond_.notify_all();
	}

	void ThreadPool::WaitUntilIdle()
	{
		{
			unique_lock<mutex> lk(tpMutex_);
			idleCond_.wait(lk, [this]() { return (taskQueue_.empty() && activeCount_ == 0); });
		}
	}
}