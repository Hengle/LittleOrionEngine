#ifndef _COMPONENTBOXCOLLIDER_H
#define _COMPONENTBOXCOLLIDER_H

#include "ComponentCollider.h"

#include <bullet3/btBulletDynamicsCommon.h>

class ComponentBoxCollider : public ComponentCollider
{
public:

	ComponentBoxCollider();
	ComponentBoxCollider(GameObject* owner);
	~ComponentBoxCollider() = default;

	Component* Clone(GameObject* owner, bool original_prefab = false) override;
	ComponentBoxCollider& operator=(const ComponentBoxCollider& component_to_copy);

	void UpdateDimensions();
	void Scale();
	void InitData();
};

#endif // !_COMPONENTBOXCOLLIDER_H
