#include "Timer.h"

namespace sludge 
{
	void Timer::Tick()
	{
		currFrameTime_ = clock_.now();

		deltaTime_ = (currFrameTime_ - prevFrameTime_).count();
		totalTime_ += deltaTime_;
		prevFrameTime_ = currFrameTime_;
	}


} // namespace