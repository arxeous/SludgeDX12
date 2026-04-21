#include "pch.h"
#include "IContext.h"

namespace sludge
{
	IContext::IContext(Config& config) : name_{ config.name }, width_{ config.width }, height_{ config.height }
	{
		aspectRatio_ = static_cast<float>(width_) / static_cast<float>(height_);
	}

}
