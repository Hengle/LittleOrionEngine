#ifndef _SKELETONIMPORTER_H_
#define _SKELETONIMPORTER_H_

#include "ResourceManagement/Importer/Importer.h"
#include "ResourceManagement/Resources/Skeleton.h"

#include <assimp/mesh.h>
#include <string>

struct aiScene;
struct aiNode;

class SkeletonImporter : public Importer
{
public:
	SkeletonImporter() : Importer(ResourceType::SKELETON) {};
	~SkeletonImporter() = default;

	FileData ExtractData(Path& assets_file_path, const Metafile& metafile) const override;
	FileData ExtractSkeletonFromAssimp(const aiScene* scene, const aiMesh* mesh, float unit_scale_factor) const;

private:
	FileData CreateBinary(const Skeleton & skeleton) const;
};

#endif