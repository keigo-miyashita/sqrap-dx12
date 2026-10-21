#include "TimelinePoint.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace sqrp
{
	TimelinePoint::TimelinePoint(QueueType queueType, UINT64 fenceVal) : queueType_(queueType), fenceVal_(fenceVal)
	{

	}

	QueueType TimelinePoint::GetQueueType() const
	{
		return queueType_;
	}

	UINT64 TimelinePoint::GetFenceValue() const
	{
		return fenceVal_;
	}
}