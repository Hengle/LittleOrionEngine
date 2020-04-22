#include "Material.h"

#include "Helper/Config.h"
#include "Main/Application.h"
#include "Module/ModuleTexture.h"
#include "Module/ModuleResourceManager.h"

#include "ResourceManagement/Metafile/Metafile.h"

Material::Material(uint32_t uuid) : Resource(uuid)
{
	textures.resize(MAX_MATERIAL_TEXTURE_TYPES);
	textures_uuid.resize(MAX_MATERIAL_TEXTURE_TYPES);
}

void Material::Save(Config& config) const
{
	for (size_t i = 0; i < textures_uuid.size(); i++)
	{
		MaterialTextureType type = static_cast<MaterialTextureType>(i);

		switch (type)
		{
		case MaterialTextureType::DIFFUSE:
			config.AddUInt(textures_uuid[i], "Diffuse");
			break;

		case MaterialTextureType::SPECULAR:
			config.AddUInt(textures_uuid[i], "Specular");
			break;

		case MaterialTextureType::OCCLUSION:
			config.AddUInt(textures_uuid[i], "Occlusion");
			break;

		case MaterialTextureType::EMISSIVE:
			config.AddUInt(textures_uuid[i], "Emissive");
			break;

		default:
			break;
		}
	}

	config.AddBool(show_checkerboard_texture, "Checkboard");
	config.AddString(shader_program, "ShaderProgram");

	//k
	config.AddFloat(k_ambient, "kAmbient");
	config.AddFloat(k_specular, "kSpecular");
	config.AddFloat(k_diffuse, "kDiffuse");
	config.AddFloat(shininess, "shininess");

	//colors
	config.AddColor(float4(diffuse_color[0], diffuse_color[1], diffuse_color[2], diffuse_color[3]), "difusseColor");
	config.AddColor(float4(emissive_color[0], emissive_color[1], emissive_color[2], 1.0f), "emissiveColor");
	config.AddColor(float4(specular_color[0], specular_color[1], specular_color[2], 1.0f), "specularColor");
}

void Material::Load(const Config& config)
{
	SetMaterialTexture(MaterialTextureType::DIFFUSE, config.GetUInt("Diffuse", 0));
	SetMaterialTexture(MaterialTextureType::SPECULAR, config.GetUInt("Specular", 0));
	SetMaterialTexture(MaterialTextureType::OCCLUSION, config.GetUInt("Occlusion", 0));
	SetMaterialTexture(MaterialTextureType::EMISSIVE, config.GetUInt("Emissive", 0));

	show_checkerboard_texture = config.GetBool("Checkboard", true);
	config.GetString("ShaderProgram", shader_program, "Blinn phong");

	//k
	k_ambient = config.GetFloat("kAmbient", 1.0f);
	k_specular = config.GetFloat("kSpecular", 1.0f);
	k_diffuse = config.GetFloat("kDiffuse", 1.0f);
	shininess = config.GetFloat("shininess", 1.0f);

	//colors
	float4 diffuse;
	float4 emissive;
	float4 specular;

	config.GetColor("difusseColor", diffuse, float4(1.f, 1.f, 1.f, 1.f));
	config.GetColor("emissiveColor", emissive, float4(0.0f, 0.0f, 0.0f, 1.0f));
	config.GetColor("specularColor", specular, float4(0.0f, 0.0f, 0.0f, 1.0f));

	diffuse_color[0] = diffuse.x;
	diffuse_color[1] = diffuse.y;
	diffuse_color[2] = diffuse.z;
	diffuse_color[3] = diffuse.w;

	emissive_color[0] = emissive.x;
	emissive_color[1] = emissive.y;
	emissive_color[2] = emissive.z;
	emissive_color[3] = emissive.w;

	specular_color[0] = specular.x;
	specular_color[1] = specular.y;
	specular_color[2] = specular.z;
	specular_color[3] = specular.w;
}

void Material::RemoveMaterialTexture(MaterialTextureType type)
{
	textures[type] = nullptr;
}

void Material::SetMaterialTexture(MaterialTextureType type, uint32_t texture_uuid)
{
	textures_uuid[type] = texture_uuid;
	if (textures_uuid[type] != 0)
	{
		textures[type] = App->resources->Load<Texture>(texture_uuid);
	}
}

const std::shared_ptr<Texture>& Material::GetMaterialTexture(MaterialTextureType type) const
{
	return textures[type];
}