#include "Timer.h"

namespace sludge 
{
	void Timer::Tick()
	{
		currFrameTime_ = clock_.now();
		numFrames_++;
		deltaTime_ = (currFrameTime_ - prevFrameTime_).count();
		totalTime_ += deltaTime_;
		prevFrameTime_ = currFrameTime_;
	}

	bool Timer::GetFPS()
	{
		if (totalTime_ > avgInterval_)
		{
			currentFPS_ = static_cast<float>(numFrames_ / totalTime_);

			if (printFPS_)
			{
				printf("FPS: %.1f\n", currentFPS_);
			}
			numFrames_ = 0;
			totalTime_ = 0;
			return true;
		}
		return false;
	}


} // namespace