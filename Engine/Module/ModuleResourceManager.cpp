#include "ModuleResourceManager.h"

#include "Component/ComponentMeshRenderer.h"
#include "Filesystem/PathAtlas.h"

#include "Helper/Config.h"
#include "Helper/Timer.h"

#include "Main/GameObject.h"
#include "Module/ModuleScene.h"

#include "ResourceManagement/Importer/Importer.h"
#include "ResourceManagement/Importer/MaterialImporter.h"
#include "ResourceManagement/Importer/ModelImporter.h"
#include "ResourceManagement/Importer/ModelImporters/AnimationImporter.h"
#include "ResourceManagement/Importer/ModelImporters/MeshImporter.h"
#include "ResourceManagement/Importer/ModelImporters/SkeletonImporter.h"
#include "ResourceManagement/Importer/PrefabImporter.h"
#include "ResourceManagement/Importer/StateMachineImporter.h"
#include "ResourceManagement/Importer/SoundImporter.h"
#include "ResourceManagement/Importer/TextureImporter.h"

#include "ResourceManagement/Manager/AnimationManager.h"
#include "ResourceManagement/Manager/FontManager.h"
#include "ResourceManagement/Manager/MaterialManager.h"
#include "ResourceManagement/Manager/MeshManager.h"
#include "ResourceManagement/Manager/PrefabManager.h"

#include "ResourceManagement/Manager/SceneManager.h"

#include "ResourceManagement/Metafile/Metafile.h"
#include "ResourceManagement/Metafile/MetafileManager.h"

#include <algorithm>
#include <Brofiler/Brofiler.h>
#include <functional> //for std::hash


ModuleResourceManager::ModuleResourceManager()
{
	resource_DB = std::make_unique<ResourceDataBase>();
}

bool ModuleResourceManager::Init()
{
	APP_LOG_SECTION("************ Module Resource Manager Init ************");

	animation_importer = std::make_unique<AnimationImporter>();
	generic_importer = std::make_unique<Importer>();
	material_importer = std::make_unique<MaterialImporter>();
	mesh_importer = std::make_unique<MeshImporter>();
	model_importer = std::make_unique<ModelImporter>();
	prefab_importer = std::make_unique<PrefabImporter>();
	skeleton_importer = std::make_unique<SkeletonImporter>();
	state_machine_importer = std::make_unique<StateMachineImporter>();
	sound_importer = std::make_unique<SoundImporter>();
	texture_importer = std::make_unique<TextureImporter>();

	metafile_manager = std::make_unique<MetafileManager>();
	scene_manager = std::make_unique<SceneManager>();


#if !GAME
	ImportAssetsInDirectory(*App->filesystem->resources_folder_path); // Import all assets in folder Resources. All metafiles in Resources are correct"
	importing_thread = std::thread(&ModuleResourceManager::StartThread, this);
#else
	App->filesystem->MountDirectory("Library");
#endif

#if MULTITHREADING
	for(size_t i = 0; i < loading_thread_communication.max_threads; ++i)
	{
		loading_thread_communication.loader_threads.push_back(std::thread(&ModuleResourceManager::LoaderThread, this));
	}
#endif

	thread_timer->Start();
	return true;
}

update_status ModuleResourceManager::PreUpdate()
{
	BROFILER_CATEGORY("PreUpdate ResourceManager", Profiler::Color::Lavender);
#if !GAME
	if (!App->time->isGameRunning() && last_imported_time > 0.0f && (thread_timer->Read() - last_imported_time) >= importer_interval_millis)
	{
		//assert(importing_thread.joinable());
		//importing_thread.join();
		//importing_thread = std::thread(&ModuleResourceManager::StartThread, this);
	}
#endif
	float t = thread_timer->Read();
	if(cache_time > 0.0f && (thread_timer->Read() - cache_time) >= cache_interval_millis)
	{
		cache_time = thread_timer->Read();
		if(!loading_thread_communication.loading)
		{

			APP_LOG_INFO("Refresh resources");
			RefreshResourceCache();
		}
	}


#if MULTITHREADING

	if(loading_thread_communication.restore_time_scale)
	{
		App->time->time_scale = 1.f;
		loading_thread_communication.loading = false;
		loading_thread_communication.restore_time_scale = false;
	}

	if(!processing_resources_queue.Empty())
	{
		LoadingJob load_job;
		if(processing_resources_queue.TryPop(load_job))
		{
			//Generate OpenGL texture
			if (load_job.component_to_load->type == Component::ComponentType::MESH_RENDERER)
			{
				load_job.component_to_load->InitResource(load_job.uuid, load_job.resource_type, load_job.texture_type);
			}
			else
			{
				load_job.component_to_load->InitResource(load_job.uuid, load_job.resource_type);
			}

			++loading_thread_communication.current_number_of_resources_loaded;
		}
	}

	if(loading_thread_communication.loading && 
		loading_thread_communication.current_number_of_resources_loaded == loading_thread_communication.total_number_of_resources_to_load)
	{
		for(const auto prefab : prefabs_to_reassign)
		{
			prefab->Reassign();
		}

		if(loading_thread_communication.current_number_of_resources_loaded != loading_thread_communication.total_number_of_resources_to_load)
		{
			//We have reassigned some prefabs resources that should be loaded
			return update_status::UPDATE_CONTINUE;
		}
		
		if(App->time->isGameRunning())
		{
			App->scene->DeleteLoadingScreen();
			App->scene->StopSceneTimer();
		}

		loading_thread_communication.restore_time_scale = true;
	}

#endif

	return update_status::UPDATE_CONTINUE;
}

bool ModuleResourceManager::CleanUp()
{
#if !GAME
	 thread_comunication.stop_thread = true;
	 importing_thread.join();
#endif
	 CleanResourceCache();

#if MULTITHREADING
	 loading_thread_communication.loading_threads_active = false;
	 for(size_t i = 0; i < loading_thread_communication.max_threads; ++i)
	 {
		 loading_thread_communication.loader_threads[i].join();
	 }
#endif

	return true;
}

 void ModuleResourceManager::StartThread()
 {
	 thread_comunication.finished_loading = false;
	 thread_comunication.total_items = App->filesystem->assets_folder_path->total_sub_files_number;

	 CleanMetafilesInDirectory(*App->filesystem->assets_folder_path); // Clean all metafiles that dont have an assets file in the folder Assets.
	 ImportAssetsInDirectory(*App->filesystem->assets_folder_path); // Import all assets in folder Assets. All metafiles in Assets have an asset file"
	 CleanBinariesInDirectory(*App->filesystem->library_folder_path); // Delete all binaries from folder Library that dont have a metafile in Assets.

	 thread_comunication.finished_loading = true;
	 last_imported_time = thread_timer->Read();
	 cache_time = thread_timer->Read();
	 first_import_completed = true;
 }

void ModuleResourceManager::CleanMetafilesInDirectory(const Path& directory_path)
{
	std::vector<Path*> files_to_delete;

	for (auto & path_child : directory_path.children)
	{
		if (thread_comunication.stop_thread)
		{
			return;
		}
		if (path_child->IsDirectory())
		{
			CleanMetafilesInDirectory(*path_child);
		}
		else if (path_child->IsMeta() && !metafile_manager->IsMetafileConsistent(*path_child))
		{
			if (metafile_manager->IsMetafileMoved(*path_child))
			{
				metafile_manager->RefreshMetafile(*path_child);
			}
			else
			{
				files_to_delete.push_back(path_child);
			}
		}
		
	}

	for (auto& path_to_delete : files_to_delete)
	{
		App->filesystem->Remove(path_to_delete->GetFullPath());
	}
}

void ModuleResourceManager::CleanBinariesInDirectory(const Path& directory_path)
{
	std::vector<Path*> files_to_delete;

	for (auto & path_child : directory_path.children)
	{
		if (thread_comunication.stop_thread)
		{
			return;
		}
		if (path_child->IsDirectory())
		{
			CleanBinariesInDirectory(*path_child);
		}
		else if (path_child->IsBinary())
		{
			uint32_t binary_uuid = std::stoul(path_child->GetFilenameWithoutExtension());
			if (!resource_DB->GetEntry(binary_uuid))
			{
				files_to_delete.push_back(path_child);
			}
		}
	}

	for (auto& path_to_delete : files_to_delete)
	{
		App->filesystem->Remove(path_to_delete->GetFullPath());
	}

	//TODO: Delete empty directories
}

void ModuleResourceManager::ImportAssetsInDirectory(const Path& directory_path, bool force)
 {
	 for (size_t i = 0; i < directory_path.children.size(); ++i)
	 {
		 Path* path_child = directory_path.children[i];
		 if (thread_comunication.stop_thread)
		 {
			 return;
		 }
		 if (path_child->IsDirectory())
		 {
			 ImportAssetsInDirectory(*path_child, force);
		 }
		 else if (path_child->IsImportable())
		 {
			 Import(*path_child, force);
			 ++thread_comunication.loaded_items;
		 }
	 }
 }

uint32_t ModuleResourceManager::Import(Path& file_path, bool force)
{
	std::lock_guard<std::mutex> lock(thread_comunication.thread_mutex);
	uint32_t imported_resource_uuid = InternalImport(file_path, force);
	return imported_resource_uuid;
}

uint32_t ModuleResourceManager::InternalImport(Path& file_path, bool force) const
{
	Metafile* asset_metafile = nullptr;

	if (force || Importer::ImportRequired(file_path))
	{
		switch (file_path.GetFile()->GetFileType())
		{
		case FileType::ANIMATION:
			asset_metafile = animation_importer->Import(file_path);
			break;

		case FileType::FONT:
			asset_metafile = generic_importer->GenericImport(file_path, ResourceType::FONT);
			break;
		
		case FileType::MATERIAL:
			asset_metafile = material_importer->Import(file_path);
			break;

		case FileType::MESH:
			asset_metafile = mesh_importer->Import(file_path);
			break;

		case FileType::MODEL:
			asset_metafile = model_importer->Import(file_path); // WARNING! If you load a model dont forget to put Prefab overwritable flag to false!!
			break;

		case FileType::PREFAB:
			asset_metafile = prefab_importer->Import(file_path);
			break;
		
		case FileType::SCENE:
			asset_metafile = generic_importer->GenericImport(file_path, ResourceType::SCENE);
			break;

		case FileType::SKELETON:
			asset_metafile = skeleton_importer->Import(file_path);
			break;

		case FileType::SKYBOX:
			asset_metafile = generic_importer->GenericImport(file_path, ResourceType::SKYBOX);
			break;

		case FileType::STATE_MACHINE:
			asset_metafile = state_machine_importer->Import(file_path);
			break;

		case FileType::TEXTURE:
			asset_metafile = texture_importer->Import(file_path);
			break;
		case FileType::SOUND:
			asset_metafile = sound_importer->Import(file_path);
			break;
		case FileType::VIDEO:
			asset_metafile = generic_importer->GenericImport(file_path, ResourceType::VIDEO);
			break;
		}
	}
	else
	{
		Path* metafile_path = App->filesystem->GetPath(metafile_manager->GetMetafilePath(file_path));
		asset_metafile = metafile_manager->GetMetafile(*metafile_path);
	}

	resource_DB->AddEntry(asset_metafile);

	return asset_metafile->uuid;
}

uint32_t ModuleResourceManager::CreateFromData(FileData data, Path& creation_folder_path, const std::string& created_resource_name)
{
	Path* created_asset_file_path = creation_folder_path.Save(created_resource_name.c_str(), data);
	return InternalImport(*created_asset_file_path);
}

uint32_t ModuleResourceManager::CreateFromData(FileData data, const std::string& created_resource_path)
{
	Path* created_asset_file_path = App->filesystem->Save(created_resource_path, data);
	return InternalImport(*created_asset_file_path);
}

void ModuleResourceManager::LoaderThread()
{
	while(loading_thread_communication.loading_threads_active)
	{
		if(!loading_resources_queue.Empty())
		{
			LoadingJob load_job;
			if(loading_resources_queue.TryPop(load_job))
			{
				//Check if resource is already on cache
				if(load_job.component_to_load->type == Component::ComponentType::MESH_RENDERER)
				{
					load_job.component_to_load->LoadResource(load_job.uuid, load_job.resource_type, load_job.texture_type);
				}
				else
				{
					load_job.component_to_load->LoadResource(load_job.uuid, load_job.resource_type);
				}
				
				processing_resources_queue.Push(load_job);
			}

			unsigned int work_load_size_remaining = loading_resources_queue.Size();

			const int ms_to_wait = work_load_size_remaining > 0 ? 10 : 2000;
			std::this_thread::sleep_for(std::chrono::milliseconds(ms_to_wait));
		}

		if(loading_resources_queue.Empty())
		{
			int hola = 0;
		}
	}
}


std::shared_ptr<Resource> ModuleResourceManager::RetrieveFromCacheIfExist(uint32_t uuid) const
{
	//Check if the resource is already loaded
	auto& it = std::find_if(resource_cache.begin(), resource_cache.end(), [&uuid](const std::shared_ptr<Resource> & resource)
	{
		return resource->GetUUID() == uuid;
	});

	if (it != resource_cache.end())
	{
		RESOURCES_LOG_INFO("Resource %u exists in cache.", uuid);
		return *it;
	}


	return nullptr;
}

bool ModuleResourceManager::RetrieveFileDataByUUID(uint32_t uuid, FileData& filedata) const
{
	std::string resource_library_file = MetafileManager::GetUUIDExportedFile(uuid);
	if (!App->filesystem->Exists(resource_library_file))
	{
		APP_LOG_ERROR("Error loading Resource %u. File %s doesn't exist", uuid, resource_library_file.c_str());
		return false;
	}

	Path* resource_exported_file_path = App->filesystem->GetPath(resource_library_file);
	filedata = resource_exported_file_path->GetFile()->Load();

	return true;
}

void ModuleResourceManager::RefreshResourceCache()
{
	//Erase Resource Cache
	resource_cache.erase(std::remove_if(resource_cache.begin(), resource_cache.end(), [](const std::shared_ptr<Resource> & resource) {
		return resource.use_count() == 1;
	}), resource_cache.end());
}
void ModuleResourceManager::AddResourceToCache(std::shared_ptr<Resource> resource)
{
	if (resource != nullptr)
	{
		resource_cache.push_back(resource);
	}
}
void ModuleResourceManager::CleanResourceCache()
{
	resource_cache.clear();
}

bool ModuleResourceManager::CleanResourceFromCache(uint32_t uuid)
{
	bool found = false;
	const auto it = std::remove_if(resource_cache.begin(), resource_cache.end(), [uuid](const std::shared_ptr<Resource> & resource) {
		return resource->GetUUID() == uuid;
	});
	if (it != resource_cache.end())
	{
		found = true;
		resource_cache.erase(it, resource_cache.end());
	}


	return found;
}
