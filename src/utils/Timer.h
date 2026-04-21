#pragma once

#include "pch.h"

namespace sludge
{
	class Timer
	{
	public:
		void Tick();

		inline double DeltaTime() const { return deltaTime_; };
		inline double TotalTime() const { return totalTime_; };

	private:
		// Our timer leverages the chrono lib
		std::chrono::high_resolution_clock  clock_;
		std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::duration<double>> currFrameTime_; // time point will get the the point in time for a given clock in whatever format we decide
		std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::duration<double>> prevFrameTime_; // in this case we want the duration in double. i.e. the time since the clocks epoch

		double deltaTime_{};
		double totalTime_{};
	};
} //namespace 