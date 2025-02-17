#ifndef _MODULECAMERA_H_
#define _MODULECAMERA_H_

#include "Module.h"
#include "Main/Globals.h"
#include "ResourceManagement/Resources/Skybox.h"
#include "Component/ComponentCanvas.h"
#include <Geometry/Frustum.h>
#include <MathGeoLib.h>


class ComponentCamera;
class GameObject;
class Skybox;

class ModuleCamera : public Module
{
public:
	ModuleCamera() = default;
	~ModuleCamera() = default;
	
	bool Init() override;
	update_status PreUpdate() override;
	update_status Update() override;
	bool CleanUp() override;
	
	ComponentCamera* CreateComponentCamera();
	void RemoveComponentCamera(ComponentCamera* camera_to_remove);

	bool IsSceneCameraMoving() const;


private:
	void SelectMainCamera();

	bool IsSceneCameraOrbiting() const;

	void SetMovement(bool movement_enabled);
	void HandleSceneCameraMovements();
	
public:
	ComponentCamera* scene_camera = nullptr;
	ComponentCamera* main_camera = nullptr;

	float aux_width = 25;
	float aux_height = 25;

	float close_mid_separation = 25;
	float mid_far_separation = 50;
	float far_plane = 75;

	std::shared_ptr<Skybox> world_skybox = nullptr;

private:
	GameObject *scene_camera_game_object = nullptr;
	


	bool movement_enabled = false;
	bool game_window_is_hovered = false;

	bool is_orbiting = false;
	bool orbit_movement_enabled = false;
	float speed_up;


	std::vector<ComponentCamera*> cameras;
	friend class ModuleEditor;

};

#endif //_MODULECAMERA_H_