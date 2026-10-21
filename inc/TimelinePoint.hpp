#pragma once

#include "pch.hpp"

#include "Device.hpp"

#include "Alias.hpp"

namespace sqrp
{
	class TimelinePoint {
	private:
		QueueType queueType_ = QueueType::Graphics;
		UINT64 fenceVal_ = 0;
	public:
		TimelinePoint(QueueType queueType, UINT64 fenceVal);
		QueueType GetQueueType() const;
		UINT64 GetFenceValue() const;
	};
}