#pragma once
#include "pch.h"

namespace sludge
{
	struct Hierarchy
	{
		int parent{ -1 };
		int firstChild{ -1 };
		int nextSibling{ -1 };
		int lastSibling{ -1 };
		int level{ 0 };
	};

	// Our scene graph implementation follows a data oriented design approach, so a "scene node" is represented by integer indices into the arrays of our scene container. 
	struct Scene
	{
		std::vector<DirectX::XMMATRIX> localTransforms{};
		std::vector<DirectX::XMMATRIX> globalTransforms{};
		std::vector<Hierarchy> hierarchy{};
		std::unordered_map<uint32_t, uint32_t> meshForNode{};
		std::unordered_map<uint32_t, uint32_t> materialForNode{};

		// debugging helpers
		std::unordered_map<uint32_t, uint32_t> nameForNode{};
		std::vector<std::string> nodeNames{};
		std::vector<std::string> modelNames{};

	};
}