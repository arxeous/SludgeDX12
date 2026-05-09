#include "Scene.h"

namespace sludge
{
	// Essentially every node that gets changed also needs to inform its children that its changed to propogate those changes down.
	void markNodeAsChanged(Scene& scene, int node)
	{
		const int level = scene.hierarchy[node].level;
		scene.changedNodesAtLevel[level].push_back(node);

		for (int firstChild = scene.hierarchy[node].firstChild; firstChild != -1; firstChild = scene.hierarchy[firstChild].nextSibling)
		{
			markNodeAsChanged(scene, firstChild);
		}
	}

	bool recalculateGlobalTransforms(Scene& scene)
	{
		bool wasUpdated = false;

		if (!scene.changedNodesAtLevel[0].empty())
		{
			const int node = scene.changedNodesAtLevel[0][0];
			scene.globalTransforms[node] = scene.localTransforms[node];
			scene.changedNodesAtLevel[0].clear();
			wasUpdated = true;
		}

		for (int i = 1; i < MAX_NODE_LEVEL; i++) 
		{
			for (int c : scene.changedNodesAtLevel[i])
			{
				const int p = scene.hierarchy[c].parent;
				scene.globalTransforms[c] = scene.globalTransforms[p] * scene.localTransforms[c];
			}
			wasUpdated |= !scene.changedNodesAtLevel[i].empty();
			scene.changedNodesAtLevel[i].clear();
		}

		return wasUpdated;
	}
} // namespace sludge