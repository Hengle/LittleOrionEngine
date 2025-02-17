#ifndef _PANELCOMPONENT_H_
#define _PANELCOMPONENT_H_

#ifndef ENGINE_EXPORTS
#define ENGINE_EXPORTS
#endif

#include "Main/Globals.h"

class Component;
class ComponentAnimation;
class ComponentAudioListener;
class ComponentAudioSource;
class ComponentBillboard;
class ComponentBoxCollider;
class ComponentButton;
class ComponentCamera;
class ComponentCanvas;
class ComponentCanvasRenderer;
class ComponentCapsuleCollider;
class ComponentCollider;
class ComponentCylinderCollider;
class ComponentEventSystem;
class ComponentImage;
class ComponentMeshCollider;
class ComponentMeshRenderer;
class ComponentTransform;
class ComponentLight;
class ComponentParticleSystem;
class ComponentSphereCollider;
class ComponentText;
class ComponentTransform;
class ComponentTrail;
class ComponentScript;
class ComponentSpriteMask;
class ComponentVideoPlayer;
class GameObject;
class PanelParticleSystem;
class PanelTrail;

class PanelComponent
{
public:
	ENGINE_API PanelComponent();
	~PanelComponent();

	void ShowComponentMeshRendererWindow(ComponentMeshRenderer* mesh_renderer);
	void ShowComponentCanvasRendererWindow(ComponentCanvasRenderer* canvas_renderer);
	void ShowComponentBillboard(ComponentBillboard* billboard);

	void ShowBillboardOptions(ComponentBillboard * billboard);

	void ShowComponentCameraWindow(ComponentCamera* camera);
	void ShowComponentLightWindow(ComponentLight* light);
	void ShowComponentAnimationWindow(ComponentAnimation* animation);
	void ShowComponentScriptWindow(ComponentScript* component_script);

	void ShowComponentCanvasWindow(ComponentCanvas* canvas);
	void ShowComponentImageWindow(ComponentImage* image);
	void ShowComponentVideoPlayerWindow(ComponentVideoPlayer* video_player);
	void ShowComponentSpriteMaskWindow(ComponentSpriteMask* component_mask);
	void ShowComponentTextWindow(ComponentText* text);
	void ShowComponentButtonWindow(ComponentButton* button);

	void ShowComponentEventSystem(ComponentEventSystem* event_system);

	void ShowComponentColliderWindow(ComponentCollider* collider);
	void ShowComponentAudioListenerWindow(ComponentAudioListener* component_audio_listener);
	void ShowComponentAudioSourceWindow(ComponentAudioSource* component_audio_source);
	void ShowComponentParticleSystem(ComponentParticleSystem* particle_system);
	void ShowComponentTrail(ComponentTrail* trail);

	void ShowAddNewComponentButton();
	bool ShowCommonComponentWindow(Component* component);

	void ShowScriptsCreated(ComponentScript* component_script);

	void CheckClickedCamera(ComponentCamera* camera);

	ENGINE_API void DropGOTarget(GameObject*& go);

private:
	bool ShowCommonColliderWindow(ComponentCollider* collider);
	void ShowComponentBoxColliderWindow(ComponentBoxCollider* box_collider);
	void ShowComponentCapsuleColliderWindow(ComponentCapsuleCollider* capsule_collider);
	void ShowComponentSphereColliderWindow(ComponentSphereCollider* sphere_collider);
	void ShowComponentCylinderColliderWindow(ComponentCylinderCollider* cylinder_collider);
	void ShowComponentMeshColliderWindow(ComponentMeshCollider* mesh_collider);

private:
	PanelParticleSystem* particle_system_panel = nullptr;
	PanelTrail* trail_panel = nullptr;
};

#endif //_PANELCOMPONENT_H_
