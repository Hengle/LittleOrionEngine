#include "PanelHierarchy.h"

#include "Actions/EditorAction.h"
#include "Actions/EditorActionDeleteGameObject.h"

#include "Component/ComponentCamera.h"
#include "Component/ComponentParticleSystem.h"
#include "Component/ComponentLight.h"

#include "EditorUI/Panel/PanelProjectExplorer.h"
#include "Helper/TemplatedGameObjectCreator.h"

#include "Main/Application.h"
#include "Main/GameObject.h"
#include "Module/ModuleActions.h"
#include "Module/ModuleCamera.h"
#include "Module/ModuleEditor.h"
#include "Module/ModuleInput.h"
#include "Module/ModuleRender.h"
#include "Module/ModuleResourceManager.h"
#include "Module/ModuleScene.h"
#include "Module/ModuleSpacePartitioning.h"

#include "ResourceManagement/Importer/Importer.h"
#include "ResourceManagement/Resources/Prefab.h"
#include "ResourceManagement/ResourcesDB/CoreResources.h"

#include <Brofiler/Brofiler.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include <FontAwesome5/IconsFontAwesome5.h>
#include <algorithm>

PanelHierarchy::PanelHierarchy()
{
	opened = true;
	enabled = true;
	window_name = ICON_FA_SITEMAP " Hierarchy";
}

void PanelHierarchy::Render()
{
	BROFILER_CATEGORY("Render Hierarchy Panel", Profiler::Color::BlueViolet);

	if (ImGui::Begin(ICON_FA_SITEMAP " Hierarchy", &opened))
	{
		hovered = ImGui::IsWindowHovered();
		focused = ImGui::IsWindowFocused();

		ImGui::SameLine();
		ImGui::Text(ICON_FA_SEARCH);

		ImGui::SameLine();
		ImGui::InputText("###GameObject Searching Input", &searching_name);
		
		if (searching_name != "")
		{
			for (unsigned int i = 0; i < App->scene->GetRoot()->children.size(); ++i)
			{
				ImGui::PushID(i);
				ShowGameObjectSearch(App->scene->GetRoot()->children[i]);
				ImGui::PopID();
			}
		}
		else 
		{
			for (unsigned int i = 0; i < App->scene->GetRoot()->children.size(); ++i)
			{
				ImGui::PushID(i);
				ShowGameObjectHierarchy(App->scene->GetRoot()->children[i]);
				ImGui::PopID();
			}
		}
		ImGui::BeginChild("Hierarchy###"); // This children window is used to create a clickable area for creating and dropping game objects in the root node
		ImGui::EndChild();
		DropTarget(App->scene->GetRoot());
		ShowGameObjectActionsMenu(nullptr);
	}
	ImGui::End();
}

void PanelHierarchy::ShowGameObjectHierarchy(GameObject* game_object)
{
	BROFILER_CATEGORY("Show GO", Profiler::Color::PaleVioletRed);
	std::string game_object_name_label;
	if (game_object->original_UUID != 0)
	{
		game_object_name_label = (std::string(ICON_FA_BOX_OPEN) + " " + game_object->name);
	}
	else 
	{
		game_object_name_label = (std::string(ICON_FA_CUBE) + " " + game_object->name);
	}
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
	if (game_object->children.size() == 0)
	{
		flags |= ImGuiTreeNodeFlags_Leaf;
	}
	
	auto it = std::find(App->editor->selected_game_objects.begin(), App->editor->selected_game_objects.end(), game_object);
	if (it != App->editor->selected_game_objects.end())
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if(App->editor->selected_game_object != nullptr && App->editor->selected_game_object != game_object)
	{
		if(IsOneOfMyChildrens(game_object))
		{

			flags |= ImGuiTreeNodeFlags_DefaultOpen;
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			window->DC.StateStorage->SetInt(window->GetID(game_object_name_label.c_str()), true);
			window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
		}
	}


	bool expanded = ImGui::TreeNodeEx(game_object_name_label.c_str(), flags);
	
	DragAndDrop(game_object);
	ShowGameObjectActionsMenu(game_object);
	ProcessMouseInput(game_object);

	if (expanded)
	{
		for (size_t i = 0; i < game_object->children.size(); i++)
		{
			ImGui::PushID(i);
			ShowGameObjectHierarchy(game_object->children[i]);
			ImGui::PopID();
		}
		ImGui::TreePop();
	}
}

void PanelHierarchy::ShowGameObjectSearch(GameObject *game_object)
{
	std::string game_object_name_label;
	if (game_object->original_UUID != 0)
	{
		game_object_name_label = (std::string(ICON_FA_BOX_OPEN) + " " + game_object->name);
	}
	else
	{
		game_object_name_label = (std::string(ICON_FA_CUBE) + " " + game_object->name);
	}

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf;

	auto it = std::find(App->editor->selected_game_objects.begin(), App->editor->selected_game_objects.end(), game_object);
	if (it != App->editor->selected_game_objects.end())
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	std::string name = game_object->name;
	transform(name.begin(), name.end(), name.begin(), ::tolower);
	transform(searching_name.begin(), searching_name.end(), searching_name.begin(), ::tolower);
	
	if (searching_name != "" && name.find(searching_name) != std::string::npos) 
	{
		ImGui::TreeNodeEx(game_object_name_label.c_str(), flags);
		ShowGameObjectActionsMenu(game_object);
		ProcessMouseInput(game_object);
		ImGui::TreePop();
	}
	
	for (size_t i = 0; i < game_object->children.size(); i++)
	{
		ImGui::PushID(i);
		ShowGameObjectSearch(game_object->children.at(i));	
		ImGui::PopID();
	}
	
	
}

void PanelHierarchy::DragAndDrop(GameObject *game_object) const
{
	DragSource(game_object);
	DropTarget(game_object);
}

void PanelHierarchy::DragSource(GameObject* source_game_object) const
{
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	{
		ImGui::SetDragDropPayload("DND_GameObject", &source_game_object, sizeof(GameObject*));
		ImGui::Text("Dragging %s", source_game_object->name.c_str());
		ImGui::EndDragDropSource();
	}
}

void PanelHierarchy::DropTarget(GameObject* target_game_object) const
{
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_GameObject"))
		{
			assert(payload->DataSize == sizeof(GameObject*));
			GameObject *incoming_game_object = *(GameObject**)payload->Data;
			bool move = true;
			bool belongs_to_list = false;
			for (auto go : App->editor->selected_game_objects) 
			{
				if (go->UUID == incoming_game_object->UUID) 
				{
					belongs_to_list = true;
				}
			}
			if (belongs_to_list) 
			{
				for (auto go : App->editor->selected_game_objects)
				{
					if (!(!go->IsAboveInHierarchy(*target_game_object) && (go->original_UUID == 0 || go->is_prefab_parent)))
					{
						move = false;
					}
				}
				if (move)
				{
					for (auto go : App->editor->selected_game_objects)
					{
						if (!App->scene->HasParentInList(go, App->editor->selected_game_objects)) 
						{
							go->SetParent(target_game_object);
						}
					}
				}
			}
			else 
			{
				if ((!incoming_game_object->IsAboveInHierarchy(*target_game_object) && (incoming_game_object->original_UUID == 0 || incoming_game_object->is_prefab_parent)))
				{
					incoming_game_object->SetParent(target_game_object);
				}
			}
		}
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_Resource"))
		{
			assert(payload->DataSize == sizeof(Metafile*));
			Metafile* incoming_metafile = *((Metafile**)payload->Data);
			if (incoming_metafile->resource_type == ResourceType::PREFAB) 
			{
				std::shared_ptr<Prefab> prefab = App->resources->Load<Prefab>(incoming_metafile->uuid);
				GameObject* new_model = prefab->Instantiate(target_game_object);

				App->actions->action_game_object = new_model;
				App->actions->AddUndoAction(ModuleActions::UndoActionType::ADD_GAMEOBJECT);
			}

			if (incoming_metafile->resource_type == ResourceType::MODEL)
			{
				std::shared_ptr<Prefab> prefab = App->resources->Load<Prefab>(incoming_metafile->uuid);
				prefab->overwritable = false;
				GameObject* new_model = prefab->Instantiate(target_game_object);

				App->actions->action_game_object = new_model;
				App->actions->AddUndoAction(ModuleActions::UndoActionType::ADD_GAMEOBJECT);
			}
		}
		ImGui::EndDragDropTarget();
	}
	
}

void PanelHierarchy::ShowGameObjectActionsMenu(GameObject* game_object)
{
	std::string label = "GameObject Creation Menu";

	if (ImGui::BeginPopupContextItem(label.c_str()))
	{
		if (game_object != nullptr)
		{
			ImGui::Text(game_object->name.c_str());
			ImGui::Separator();
		}

		if (game_object != nullptr)
		{
			if (ImGui::Selectable("Delete GameObject"))
			{
				bool belongs_to_list = false;
				for (auto go : App->editor->selected_game_objects)
				{
					if (go->UUID == game_object->UUID) 
					{
						belongs_to_list = true;
					}
				}
				if (belongs_to_list) 
				{
					App->actions->AddUndoAction(ModuleActions::UndoActionType::DELETE_MULTIPLE_GO);
					for (auto go : App->editor->selected_game_objects)
					{
						App->scene->RemoveGameObject(go);
					}
					App->editor->selected_game_object = nullptr;
					App->editor->selected_game_objects.erase(App->editor->selected_game_objects.begin(), App->editor->selected_game_objects.end());

				}
				else 
				{
					App->actions->action_game_object = game_object;
					App->actions->AddUndoAction(ModuleActions::UndoActionType::DELETE_GAMEOBJECT);
					App->scene->RemoveGameObject(game_object);
					App->editor->selected_game_object = nullptr;
				}
			}
			if(ImGui::Selectable("Duplicate"))
			{
				bool belongs_to_list = false;
				for (auto go : App->editor->selected_game_objects)
				{
					if (go->UUID == game_object->UUID) 
					{
						belongs_to_list = true;
					}
				}
				if (belongs_to_list) 
				{
					App->actions->AddUndoAction(ModuleActions::UndoActionType::ADD_MULTIPLE_GO);
					App->scene->DuplicateGameObjectList(App->editor->selected_game_objects);
				}
				else 
				{
					GameObject* duplicated_go = App->scene->DuplicateGameObject(game_object, game_object->parent);
					App->actions->action_game_object = duplicated_go;
					App->actions->AddUndoAction(ModuleActions::UndoActionType::ADD_GAMEOBJECT);
				}
			}
			

			if (ImGui::Selectable("Move Up"))
			{
				game_object->MoveUpInHierarchy();
			}
			
			if (ImGui::Selectable("Sort by name"))
			{
				App->scene->SortGameObjectChilds(App->scene->GetRoot());
			}

			if (ImGui::Selectable("Move Down"))
			{
				game_object->MoveDownInHierarchy();
			}

			if (ImGui::Selectable("Select Children"))
			{
				App->editor->selected_game_objects.clear();
				for (auto go : game_object->children) 
				{
					App->editor->selected_game_objects.push_back(go);
				}
			}

			if (game_object->prefab_reference && ImGui::Selectable("Unpack Prefab"))
			{
				GameObject * prefab_parent = game_object->GetPrefabParent();
				prefab_parent->UnpackPrefab();

			}else if(!game_object->prefab_reference && ImGui::Selectable("Create Prefab"))
			{
				App->editor->project_explorer->CreatePrefabInSelectedFolder(game_object);
			}
			ImGui::Separator();
		}

		if (ImGui::Selectable("Create Empty"))
		{
			GameObject* created_go = game_object != nullptr ? App->scene->CreateChildGameObject(game_object) : App->scene->CreateGameObject();
			App->actions->action_game_object = created_go;
			App->actions->AddUndoAction(ModuleActions::UndoActionType::ADD_GAMEOBJECT);
		}
		
		Show3DObjectCreationMenu(game_object);
		ShowComponentObjectCreationMenu(game_object);
		ImGui::EndPopup();
	}
}

void PanelHierarchy::Show3DObjectCreationMenu(GameObject* game_object) const
{
	if (ImGui::BeginMenu("3D object"))
	{
		GameObject* created_game_object = nullptr;
		if (ImGui::Selectable("Cube"))
		{
			created_game_object = TemplatedGameObjectCreator::CreatePrimitive(CoreResource::CUBE);
		}
		if (ImGui::Selectable("Cylinder"))
		{
			created_game_object = TemplatedGameObjectCreator::CreatePrimitive(CoreResource::CYLINDER);
		}
		if (ImGui::Selectable("Sphere"))
		{
			created_game_object = TemplatedGameObjectCreator::CreatePrimitive(CoreResource::SPHERE);
		}
		if (ImGui::Selectable("Torus"))
		{
			created_game_object = TemplatedGameObjectCreator::CreatePrimitive(CoreResource::TORUS);
		}
		if (ImGui::Selectable("Quad"))
		{
			created_game_object = TemplatedGameObjectCreator::CreatePrimitive(CoreResource::QUAD);
		}

		if (game_object != nullptr && created_game_object != nullptr)
		{
			created_game_object->SetParent(game_object);
		}

		if(created_game_object != nullptr)
		{
			App->space_partitioning->InsertAABBTree(created_game_object);
		}

		ImGui::EndMenu();
	}
}

void PanelHierarchy::ShowComponentObjectCreationMenu(GameObject* game_object) const
{
	GameObject* created_game_object = nullptr;

	if (ImGui::BeginMenu("Light"))
	{
		if (ImGui::Selectable("Point Light"))
		{
			created_game_object = App->scene->CreateGameObject();
			created_game_object->name = "Point Light";
			ComponentLight* point_light_component = static_cast<ComponentLight*>(created_game_object->CreateComponent(Component::ComponentType::LIGHT));
			point_light_component->light_type = ComponentLight::LightType::POINT_LIGHT;
		}
		if (ImGui::Selectable("Spot Light"))
		{
			created_game_object = App->scene->CreateGameObject();
			created_game_object->name = "Spot Light";
			ComponentLight* spot_light_component = static_cast<ComponentLight*>(created_game_object->CreateComponent(Component::ComponentType::LIGHT));
			spot_light_component->light_type = ComponentLight::LightType::SPOT_LIGHT;
		}
		if (ImGui::Selectable("Directional Light"))
		{
			created_game_object = App->scene->CreateGameObject();
			created_game_object->name = "Directional Light";
			ComponentLight* directional_light_component = static_cast<ComponentLight*>(created_game_object->CreateComponent(Component::ComponentType::LIGHT));
			directional_light_component->light_type = ComponentLight::LightType::DIRECTIONAL_LIGHT;
		}

		ImGui::EndMenu();
	}
	
	if (ImGui::BeginMenu("Effects"))
	{
		if (ImGui::Selectable("Particle System"))
		{
			created_game_object = App->scene->CreateGameObject();
			created_game_object->name = "Particle System";
			ComponentParticleSystem* particle_system_component = static_cast<ComponentParticleSystem*>(created_game_object->CreateComponent(Component::ComponentType::PARTICLE_SYSTEM));
		}

		if (ImGui::Selectable("Trail"))
		{
			GameObject* created_game_object = App->scene->CreateGameObject();
			created_game_object->name = "Trail";
			created_game_object->CreateComponent(Component::ComponentType::TRAIL);
		}

		ImGui::EndMenu();
	}

	if (ImGui::Selectable("Camera"))
	{
		created_game_object = App->scene->CreateGameObject();
		created_game_object->name = "Camera";
		created_game_object->CreateComponent(Component::ComponentType::CAMERA);
	}

	if (ImGui::BeginMenu("UI"))
	{

		if (ImGui::Selectable("Text"))
		{
			created_game_object = TemplatedGameObjectCreator::CreateUIText();
		}
		if (ImGui::Selectable("Image"))
		{
			created_game_object = TemplatedGameObjectCreator::CreateUIImage();
		}
		ImGui::Separator();
		if (ImGui::Selectable("Button"))
		{
			created_game_object = TemplatedGameObjectCreator::CreateUIButton();

		}
		ImGui::EndMenu();
	}

	if (game_object != nullptr && created_game_object != nullptr)
	{
		created_game_object->SetParent(game_object);
	}
}

void PanelHierarchy::ProcessMouseInput(GameObject* game_object)
{
	if (ImGui::IsItemHovered())
	{
		if (App->input->GetMouseButtonUp(MouseButton::Left))
		{
			control_key_down = true;
		}
		if (App->input->GetKeyUp(KeyCode::LeftControl))
		{
			control_key_down = false;
		}
		if (control_key_down && App->input->GetKey(KeyCode::LeftControl))
		{
			App->editor->selected_game_object = game_object;
			bool already_selected = false;
			for (auto go : App->editor->selected_game_objects)
			{
				if ((go->UUID == game_object->UUID))
				{
					already_selected = true;
				}
			}
			if (!already_selected)
			{
				App->editor->selected_game_objects.push_back(game_object);
			}

			App->editor->show_game_object_inspector = true;
			control_key_down = false;
		}
		else 
		{
			if (control_key_down)
			{
				App->editor->selected_game_object = game_object;
				App->editor->selected_game_objects.erase(App->editor->selected_game_objects.begin(), App->editor->selected_game_objects.end());
				App->editor->selected_game_objects.push_back(game_object);
				App->editor->show_game_object_inspector = true;
				control_key_down = false;
			}
		}
		

		if (ImGui::IsMouseDoubleClicked(0))
		{
			App->cameras->scene_camera->Center(game_object->aabb.global_bounding_box);
		}
	}
}

std::string PanelHierarchy::GetNextGameObjectName()
{
	char tmp_string[64];
	sprintf_s(tmp_string, "GameObject (%d)", num_game_objects++);

	return std::string(tmp_string);
}

int PanelHierarchy::GetNextBranch()
{
	return ++branch_counter;
}

bool PanelHierarchy::IsOneOfMyChildrens(GameObject* game_object) const
{
	bool found = false;
	
	for (auto& go : game_object->children) 
	{
		found = std::find(go->children.begin(), go->children.end(), App->editor->selected_game_object) != go->children.end() || go == App->editor->selected_game_object;
		if (!found && go->children.size() > 0)
		{
			found = IsOneOfMyChildrens(go);
		}
		if(found)
		{
			break;
		}
	}
	return found;
}