#include "DeferredRelease.hpp"

using namespace std;

namespace sqrp
{
	void DeferredRelease::ReleaseCompleted(UINT64 completedGraphicsFenceValue, UINT64 completedComputeFenceValue)
	{
		vector<Entry> toRelease;
		{
			lock_guard<mutex> lock(mutex_);
			while (!deferredReleaseQueue_.empty()
				   && deferredReleaseQueue_.front().graphicsFenceValue <= completedGraphicsFenceValue
				   && deferredReleaseQueue_.front().computeFenceValue <= completedComputeFenceValue) {
				toRelease.push_back(std::move(deferredReleaseQueue_.front()));
				deferredReleaseQueue_.pop_front();
			}
		}
	}

	void DeferredRelease::ReleaseAll()
	{
		while (true) {
			deque<Entry> toRelease;
			{
				lock_guard<mutex> lock(mutex_);
				if (deferredReleaseQueue_.empty()) {
					break;
				}
				toRelease.swap(deferredReleaseQueue_);
			}
		}
	}
}