#ifndef _MODULEEDITOR_H_
#define _MODULEEDITOR_H_

#ifndef ENGINE_EXPORTS
#define ENGINE_EXPORTS
#endif

#include "Module.h"
#include "Main/Globals.h"
#include "ResourceManagement/Resources/Texture.h"

#include <vector>
#include <ImGuizmo.h>
#include <MathGeoLib.h>

class Component;

class GameObject;

class Panel;
class PanelBuildOptions;
class PanelMenuBar;
class PanelToolBar;
class PanelScene;
class PanelStateMachine;
class PanelGame;
class PanelInspector;
class PanelHierarchy;
class PanelProjectExplorer;
class PanelConsole;
class PanelDebug;
class PanelConfiguration;
class PanelAbout;
class PanelResourceDatabase;
class PanelPopups;
class PanelNavMesh;
class PanelTags;

class TagManager;

struct ImFont;

class ModuleEditor : public Module
{
public:

	ModuleEditor() = default;
	~ModuleEditor() = default;

	bool Init() override;
	update_status PreUpdate() override;
	update_status Update() override;
	bool CleanUp() override;

	void Render();

	ImFont* GetFont(const Fonts & font) const;
	ENGINE_API ImGuiContext* GetImGuiContext() const;
private:
	void InitEditorDockspace();
	void RenderEditorDockspace();

	bool InitImgui();
	void LoadFonts();

	

public:
	GameObject *selected_game_object = nullptr;
	std::vector<GameObject*> selected_game_objects;
	bool show_game_object_inspector = true;

	float3 selected_position = float3(0.0f, 0.0f, 0.0f);
	ImGuizmo::OPERATION gizmo_operation = ImGuizmo::TRANSLATE;
	
	Metafile *selected_meta_file = nullptr;

	TagManager* tag_manager = nullptr;

	PanelBuildOptions* build_options = nullptr;
	PanelMenuBar* menu_bar = nullptr;
	PanelToolBar* toolbar = nullptr;
	PanelScene* scene_panel = nullptr;
	PanelGame* game_panel = nullptr;
	PanelInspector* inspector = nullptr;
	PanelHierarchy* hierarchy = nullptr;
	PanelProjectExplorer* project_explorer = nullptr;
	PanelConsole* console = nullptr;
	PanelDebug* debug_panel = nullptr;
	PanelConfiguration* configuration = nullptr;
	PanelAbout* about = nullptr;
	PanelResourceDatabase* resource_database = nullptr;
	PanelPopups* popups = nullptr;
	PanelNavMesh* nav_mesh = nullptr;
	PanelStateMachine* state_machine = nullptr;
	PanelTags* tags_panel = nullptr;

private:
	std::vector<Panel*> panels;
	ImGuiID editor_dockspace_id;
	ImGuiContext* imgui_context = nullptr;	
};

#endif //_MODULEEDITOR_H_