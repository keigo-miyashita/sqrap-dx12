#pragma once

#include "pch.hpp"

namespace sqrp
{
	class DeferredRelease
	{
	private:
		struct Entry {
			UINT64 graphicsFenceValue;
			UINT64 computeFenceValue;
			std::unique_ptr<void, void(*)(void*)> resource;
		};
		std::mutex mutex_;
		std::deque<Entry> deferredReleaseQueue_;

	public:
		template<typename T>
		void Push(UINT64 graphicsFenceValue, UINT64 computeFenceValue, T* resource)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			deferredReleaseQueue_.push_back(
				{ graphicsFenceValue, computeFenceValue,
				std::unique_ptr<void, void(*)(void*)>(resource, [](void* p) { delete static_cast<T*>(p); }) 
				}
			);
		}
		void ReleaseCompleted(UINT64 completedGraphicsFenceValue, UINT64 completedComputeFenceValue);
		void ReleaseAll();
	};
}
