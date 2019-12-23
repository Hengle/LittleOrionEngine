#include "ModuleFileSystem.h"
#include "Application.h"
#include <algorithm>
#include <SDL/SDL.h>


bool ModuleFileSystem::Init() {
	MakeDirectory(".//", "Assets");
	MakeDirectory(".//", "Library");
	MakeDirectory(".//Library", "Materials");
	MakeDirectory(".//Library", "Meshes");
	RefreshFilesHierarchy();
	return true;
}
char* ModuleFileSystem::Load(const char* file_path, size_t & size) const
{

	SDL_RWops *rw = SDL_RWFromFile(file_path, "rb");
	if (rw == NULL) return NULL;

	Sint64 res_size = SDL_RWsize(rw);
	char* res = (char*)malloc(res_size + 1);

	Sint64 nb_read_total = 0, nb_read = 1;
	char* buf = res;
	while (nb_read_total < res_size && nb_read != 0) {
		nb_read = SDL_RWread(rw, buf, 1, (res_size - nb_read_total));
		nb_read_total += nb_read;
		buf += nb_read;
	}
	SDL_RWclose(rw);
	if (nb_read_total != res_size) {
		free(res);
		return NULL;
	}

	res[nb_read_total] = '\0';
	if (size != NULL)
		size = nb_read_total;
	return res;

}
unsigned int ModuleFileSystem::Save(const char* file_path, const void* buffer, unsigned int size, bool append) const
{
	SDL_RWops* file;
	if (append) 
	{
		file = SDL_RWFromFile(file_path, "a+b");
	}
	else 
	{
		file = SDL_RWFromFile(file_path, "w+b");
	}

	if (file != NULL)
	{
		APP_LOG_INFO("File %s opened!\n", file_path);

	SDL_RWwrite(file, buffer, size, 1);

	}
	else
	{
		APP_LOG_ERROR("Error: Unable to open file! SDL Error: %s\n", SDL_GetError());
		return 1;
	}
	SDL_RWclose(file);
	return 0;
}
bool ModuleFileSystem::Remove(const File & file) const
{
	if (file.file_type == FileType::DIRECTORY) {
		return RemoveDirectoryA(file.file_path.c_str());
	}
	return DeleteFileA(file.file_path.c_str());
}
bool ModuleFileSystem::Exists(const char* file_path) const
{
	SDL_RWops* file = SDL_RWFromFile(file_path, "r");
	bool exists = file != NULL;
	SDL_RWclose(file);
	return exists;
}

std::string ModuleFileSystem::MakeDirectory(const std::string & path, const std::string & directory_name)
{
	std::string new_directory = path + "//" + directory_name;
	CreateDirectory(new_directory.c_str(), NULL);
	return new_directory;
}
bool ModuleFileSystem::Copy(const char* source, const char* destination)
{
	return true;
}

ModuleFileSystem::FileType ModuleFileSystem::GetFileType(const char *file_path, const DWORD & file_attributes) const
{

	std::string file_extension = GetFileExtension(file_path);
	if ((file_attributes & FILE_ATTRIBUTE_DIRECTORY ) || (file_extension == "/" ))
	{
		return ModuleFileSystem::FileType::DIRECTORY;
	}

	if (
		file_extension == "png"
		|| file_extension == "PNG"
		|| file_extension == "dds"
		|| file_extension == "DDS"
		|| file_extension == "tga"
		|| file_extension == "JPG"
		)
	{
		return ModuleFileSystem::FileType::TEXTURE;
	}
	if (
		file_extension == "fbx"
		|| file_extension == "FBX"
		)
	{
		return ModuleFileSystem::FileType::MODEL;
	}
	if (file_extension == "" && file_attributes & FILE_ATTRIBUTE_ARCHIVE)
	{
		return ModuleFileSystem::FileType::ARCHIVE;
	}
	return ModuleFileSystem::FileType::UNKNOWN;
}


std::string ModuleFileSystem::GetFileExtension(const char *file_path) const
{
	std::string file_path_string = std::string(file_path);
	if (file_path_string.back() == '/') 
	{
		return std::string("/");
	}

	std::size_t found = file_path_string.find_last_of(".");
	if (found == std::string::npos || found == 0) {
		return "";
	}
	std::string file_extension = file_path_string.substr(found + 1, file_path_string.length());

	return file_extension;
}

void ModuleFileSystem::GetAllFilesInPath(const std::string & path, std::vector<std::shared_ptr<File>> & files, bool directories_only) const
{
	std::string path_all = path + "//*";

	WIN32_FIND_DATA find_file_data;
	HANDLE handle_find = FindFirstFile(path_all.c_str(), &find_file_data);
	if (handle_find == INVALID_HANDLE_VALUE) {
		FindClose(handle_find);
		return;
	}
	do {
		bool is_directory = find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
		if (IsValidFileName(find_file_data.cFileName) && ((directories_only && is_directory) || !directories_only))
		{
			std::shared_ptr<File> new_file = std::make_shared<File>(find_file_data,path);
			files.push_back(new_file);
		}
	} while (FindNextFile(handle_find, &find_file_data) != 0);

	FindClose(handle_find);
}

std::shared_ptr<ModuleFileSystem::File> ModuleFileSystem::GetFileHierarchyFromPath(const std::string & path) const
{
	std::shared_ptr<File> new_file = std::make_shared<File>();
	new_file->file_path = path;
	GetAllFilesRecursive(new_file);
	return new_file;
}
void ModuleFileSystem::GetAllFilesRecursive(std::shared_ptr<File> root) const
{
	std::vector<std::shared_ptr<File>> files;
	GetAllFilesInPath(root->file_path, files, true);
	for (auto & file : files )
	{
		file->parent = root;
		root->childs.push_back(file);
		GetAllFilesRecursive(file);

	}
}

size_t ModuleFileSystem::GetNumberOfSubFolders(const std::string & path) const
{
	std::string path_all = path + "//*";
	WIN32_FIND_DATA find_file_data;
	HANDLE handle_find = FindFirstFile(path_all.c_str(), &find_file_data);

	if (handle_find == INVALID_HANDLE_VALUE) {
		FindClose(handle_find);
		return 0;
	}
	size_t subFiles = 0;
	do {
		if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && IsValidFileName(find_file_data.cFileName))
		{
			++subFiles;
		}

	} while (FindNextFile(handle_find, &find_file_data) != 0);

	FindClose(handle_find);
	return subFiles;
}

bool ModuleFileSystem::IsValidFileName(const char * file_name) const
{
	return std::strcmp(file_name, ".") && std::strcmp(file_name, "..");
}

void ModuleFileSystem::RefreshFilesHierarchy()
{
	root_file = GetFileHierarchyFromPath(".//Assets");
}

bool ModuleFileSystem::File::operator==(const ModuleFileSystem::File& compare)
{
	return this->filename == compare.filename && this->file_path == compare.file_path && this->file_type == compare.file_type;
};
ModuleFileSystem::File::File(const WIN32_FIND_DATA & windows_file_data, const std::string & path) {
	this->filename = windows_file_data.cFileName;
	this->file_path = path + "//" + windows_file_data.cFileName;
	this->file_type = App->filesystem->GetFileType(filename.c_str(), windows_file_data.dwFileAttributes);
	this->filename_no_extension = this->filename.substr(0, this->filename.find_last_of("."));
}

ModuleFileSystem::File::File(const std::string & path) {

	WIN32_FIND_DATA find_file_data;
	HANDLE handle_find = FindFirstFile(path.c_str(), &find_file_data);
	if (handle_find == INVALID_HANDLE_VALUE) {
		FindClose(handle_find);
		return;
	}
	this->filename = find_file_data.cFileName;
	this->file_path = path;
	this->file_type = App->filesystem->GetFileType(filename.c_str(), find_file_data.dwFileAttributes);
	this->filename_no_extension = this->filename.substr(0, this->filename.find_last_of("."));
	std::replace(this->file_path.begin(), this->file_path.end(), '\\', '\/');

}

