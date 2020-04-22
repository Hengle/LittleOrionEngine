﻿#include "ComponentMeshRenderer.h"

#include "ComponentAnimation.h"
#include "Main/Application.h"
#include "Main/GameObject.h"
#include "Module/ModuleLight.h"
#include "Module/ModuleProgram.h"
#include "Module/ModuleRender.h"
#include "Module/ModuleResourceManager.h"
#include "Module/ModuleTexture.h"
#include "Module/ModuleScene.h"

#include "ResourceManagement/ResourcesDB/CoreResources.h"

ComponentMeshRenderer::ComponentMeshRenderer(GameObject * owner) : Component(owner, ComponentType::MESH_RENDERER)
{
	SetMesh(0);
	SetMaterial(0);
	owner->aabb.GenerateBoundingBox();
}

ComponentMeshRenderer::ComponentMeshRenderer() : Component(nullptr, ComponentType::MESH_RENDERER)
{
	SetMesh(0);
	SetMaterial(0);
}


void ComponentMeshRenderer::Delete()
{
	App->renderer->RemoveComponentMesh(this);
}

void ComponentMeshRenderer::Save(Config& config) const
{
	config.AddUInt(UUID, "UUID");
	config.AddInt((unsigned int)type, "ComponentType");
	config.AddBool(active, "Active");
	config.AddUInt(mesh_uuid, "Mesh");
	config.AddUInt(material_uuid, "Material");
	config.AddUInt(skeleton_uuid, "Skeleton");
}

void ComponentMeshRenderer::Load(const Config& config)
{
	UUID = config.GetUInt("UUID", 0);
	active = config.GetBool("Active", true);

	mesh_uuid = config.GetUInt("Mesh", 0);
	SetMesh(mesh_uuid);

	material_uuid = config.GetUInt("Material", 0);
	SetMaterial(material_uuid);

	skeleton_uuid =	config.GetUInt("Skeleton", 0);
	SetSkeleton(skeleton_uuid);
}

void ComponentMeshRenderer::Render()
{
	if (material_to_render == nullptr)
	{
		return;
	}

	std::string program_name = material_to_render->shader_program;
	GLuint program = App->program->GetShaderProgramId(program_name);
	glUseProgram(program);

	if (palette.size() > 0)
	{
		glUniformMatrix4fv(glGetUniformLocation(program, "palette"), palette.size(), GL_TRUE, &palette[0][0][0]);
	}

	glBindBuffer(GL_UNIFORM_BUFFER, App->program->uniform_buffer.ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, App->program->uniform_buffer.MATRICES_UNIFORMS_OFFSET, sizeof(float4x4), owner->transform.GetGlobalModelMatrix().Transposed().ptr());
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	App->lights->Render(owner->transform.GetGlobalTranslation(), program);
	RenderMaterial(program);
	RenderModel();
	glUseProgram(0);
}	

void ComponentMeshRenderer::RenderModel() const
{
	if (mesh_to_render == nullptr)
	{
		return;
	}	
	glBindVertexArray(mesh_to_render->GetVAO());
	glDrawElements(GL_TRIANGLES, mesh_to_render->indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void ComponentMeshRenderer::RenderMaterial(GLuint shader_program) const
{
	AddDiffuseUniforms(shader_program);
	AddEmissiveUniforms(shader_program);
	AddSpecularUniforms(shader_program);
	AddAmbientOclusionUniforms(shader_program);
}


void ComponentMeshRenderer::AddDiffuseUniforms(unsigned int shader_program) const
{
	glActiveTexture(GL_TEXTURE0);
	BindTexture(Material::MaterialTextureType::DIFFUSE);
	glUniform1i(glGetUniformLocation(shader_program, "material.diffuse_map"), 0);
	glUniform4fv(glGetUniformLocation(shader_program, "material.diffuse_color"), 1, (float*)material_to_render->diffuse_color);
	glUniform1f(glGetUniformLocation(shader_program, "material.k_diffuse"), material_to_render->k_diffuse);

}

void ComponentMeshRenderer::AddEmissiveUniforms(unsigned int shader_program) const
{
	glActiveTexture(GL_TEXTURE1);
	BindTexture(Material::MaterialTextureType::EMISSIVE);
	glUniform1i(glGetUniformLocation(shader_program, "material.emissive_map"), 1);
	glUniform4fv(glGetUniformLocation(shader_program, "material.emissive_color"), 1, (float*)material_to_render->emissive_color);
}

void ComponentMeshRenderer::AddSpecularUniforms(unsigned int shader_program) const
{
	glActiveTexture(GL_TEXTURE2);
	BindTexture(Material::MaterialTextureType::SPECULAR);
	glUniform1i(glGetUniformLocation(shader_program, "material.specular_map"), 2);
	glUniform4fv(glGetUniformLocation(shader_program, "material.specular_color"), 1, (float*)material_to_render->specular_color);
	glUniform1f(glGetUniformLocation(shader_program, "material.k_specular"), material_to_render->k_specular);
	glUniform1f(glGetUniformLocation(shader_program, "material.shininess"), material_to_render->shininess);
}

void ComponentMeshRenderer::AddAmbientOclusionUniforms(unsigned int shader_program) const
{
	glActiveTexture(GL_TEXTURE3);
	BindTexture(Material::MaterialTextureType::OCCLUSION);
	glUniform1i(glGetUniformLocation(shader_program, "material.occlusion_map"), 3);
	glUniform1f(glGetUniformLocation(shader_program, "material.k_ambient"), material_to_render->k_ambient);
}

void ComponentMeshRenderer::BindTexture(Material::MaterialTextureType id) const
{
	GLuint texture_id;
	if (material_to_render->show_checkerboard_texture)
	{
		texture_id = App->texture->checkerboard_texture_id;
	}
	else if (material_to_render->textures[id] != nullptr)
	{
		texture_id = material_to_render->textures[id]->opengl_texture;
	}
	else
	{
		texture_id = App->texture->whitefall_texture_id;
	}
	glBindTexture(GL_TEXTURE_2D, texture_id);
}

Component* ComponentMeshRenderer::Clone(bool original_prefab) const
{
	ComponentMeshRenderer * created_component;
	if (original_prefab)
	{
		created_component = new ComponentMeshRenderer();
	}
	else
	{
		created_component = App->renderer->CreateComponentMeshRenderer();
	}
	*created_component = *this;
	return created_component;
}

void ComponentMeshRenderer::Copy(Component* component_to_copy) const
{
	*component_to_copy = *this;
	*static_cast<ComponentMeshRenderer*>(component_to_copy) = *this;
};

void ComponentMeshRenderer::SetMesh(uint32_t mesh_uuid)
{
	this->mesh_uuid = mesh_uuid;
	if (mesh_uuid != 0)
	{
		this->mesh_to_render = App->resources->Load<Mesh>(mesh_uuid);
		owner->aabb.GenerateBoundingBox();
	}
}

void ComponentMeshRenderer::SetMaterial(uint32_t material_uuid)
{
	this->material_uuid = material_uuid;
	if (material_uuid != 0)
	{
		material_to_render = App->resources->Load<Material>(material_uuid);
	}
	else
	{
		material_to_render = App->resources->Load<Material>((uint32_t)CoreResource::DEFAULT_MATERIAL);
	}
}

void ComponentMeshRenderer::SetSkeleton(uint32_t skeleton_uuid)
{
	this->skeleton_uuid = skeleton_uuid;
	if (skeleton_uuid != 0)
	{
		skeleton = App->resources->Load<Skeleton>(skeleton_uuid);
		palette.resize(skeleton->skeleton.size());
		for (auto & matrix : palette)
		{
			matrix = float4x4::identity;
		}
	}
}

void ComponentMeshRenderer::UpdatePalette(const std::vector<float4x4>& pose)
{
	assert(pose.size() == palette.size());
	for (size_t i = 0; i < pose.size(); ++i)
	{
		auto &  joints = skeleton->skeleton;
		size_t joint_index = i;
		float4x4 gobal_transform = float4x4::identity;
		while (joints[joint_index].parent_index != -1)
		{
			joint_index = joints[joint_index].parent_index;
			gobal_transform = pose[joint_index] * gobal_transform;

		}
		palette[i] =  gobal_transform * pose[i] * joints[i].transform_global;
	}
}