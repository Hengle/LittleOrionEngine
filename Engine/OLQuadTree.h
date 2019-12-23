#ifndef _OLQUADTREE_H_
#define _OLQUADTREE_H_

#include "GameObject.h"
#include "OLQuadTreeNode.h"

#include <MathGeoLib.h>

class OLQuadTree
{
public:
	OLQuadTree();
	~OLQuadTree();

	void Create(AABB2D limits);
	void Clear();
	void Insert(GameObject &game_object);
	void CollectIntersect(std::vector<GameObject*> &game_objects, const ComponentCamera &camera);

	static std::vector<float> GetVertices(const AABB2D &box); // TODO: Move to Utils class

public:
	std::vector<OLQuadTreeNode*> flattened_tree;

private:
	void FindLeaves(const AABB2D &game_object, std::vector<OLQuadTreeNode*> &leaves) const;

private:
	OLQuadTreeNode *root = nullptr;
	int bucket_size = 3;
	int max_depth = 3;
	friend class ModuleDebug;
};

#endif //_OLQUADTREE_H_