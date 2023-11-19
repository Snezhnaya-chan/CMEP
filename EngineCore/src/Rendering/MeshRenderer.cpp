#include <assert.h>
#include <sstream>
#include <fstream>
#include <cstring>

#include "Rendering/MeshRenderer.hpp"
#include "SceneManager.hpp"
#include "Rendering/Texture.hpp"
#include <glm/gtx/quaternion.hpp>
#include "Logging/Logging.hpp"

#include "Engine.hpp"

namespace Engine::Rendering
{
	MeshRenderer::MeshRenderer(Engine* engine) : IRenderer(engine)
	{
		VulkanRenderingEngine* renderer = this->owner_engine->GetRenderingEngine();

		VulkanPipelineSettings pipeline_settings = renderer->getVulkanDefaultPipelineSettings();
		pipeline_settings.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		
		pipeline_settings.descriptorLayoutSettings.binding.push_back(0);
		pipeline_settings.descriptorLayoutSettings.descriptorCount.push_back(1);
		pipeline_settings.descriptorLayoutSettings.types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		pipeline_settings.descriptorLayoutSettings.stageFlags.push_back(VK_SHADER_STAGE_VERTEX_BIT);

		pipeline_settings.descriptorLayoutSettings.binding.push_back(1);
		pipeline_settings.descriptorLayoutSettings.descriptorCount.push_back(16);
		pipeline_settings.descriptorLayoutSettings.types.push_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		pipeline_settings.descriptorLayoutSettings.stageFlags.push_back(VK_SHADER_STAGE_FRAGMENT_BIT);

		this->pipeline = renderer->createVulkanPipeline(pipeline_settings, "game/shaders/vulkan/meshrenderer_vert.spv", "game/shaders/vulkan/meshrenderer_frag.spv");

		this->matMVP = glm::mat4();
	}

	MeshRenderer::~MeshRenderer()
	{
		this->logger->SimpleLog(Logging::LogLevel::Debug3, "Cleaning up mesh renderer");
		VulkanRenderingEngine* renderer = this->owner_engine->GetRenderingEngine();

		vkDeviceWaitIdle(renderer->GetLogicalDevice());
		
		if(this->vbo != nullptr)
		{
			renderer->cleanupVulkanBuffer(this->vbo);
		}
		renderer->cleanupVulkanPipeline(this->pipeline);
	}

	void MeshRenderer::AssignMesh(std::shared_ptr<Mesh> new_mesh)
	{
		this->mesh = new_mesh;

		this->has_updated_mesh = false;
		this->has_updated_meshdata = false;
	}

	void MeshRenderer::UpdateTexture(const Rendering::Texture* texture)
	{
		this->texture.reset(texture);

		this->has_updated_mesh = false;
	}

	void MeshRenderer::Update(glm::vec3 pos, glm::vec3 size, glm::vec3 rotation, uint_fast16_t screenx, uint_fast16_t screeny, glm::vec3 parent_position, glm::vec3 parent_rotation, glm::vec3 parent_size)
	{
		this->_pos = pos;
		this->_size = size;
		this->_rotation = rotation;

		this->_parent_pos = parent_position;
		this->_parent_rotation = parent_rotation;
		this->_parent_size = parent_size;

		this->_screenx = screenx;
		this->_screeny = screeny;

		this->has_updated_mesh = false;
	}

	void MeshRenderer::UpdateMesh()
	{
		if (!this->mesh)
		{
			return;
		}

		this->has_updated_mesh = true;

		VulkanRenderingEngine* renderer = this->owner_engine->GetRenderingEngine();

		glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (float)this->_screenx / this->_screeny, 0.1f, 100.0f);
		Projection[1][1] *= -1;

		glm::mat4 View;
		if(auto locked_scene_manager = this->scene_manager.lock())
		{
			View = locked_scene_manager->GetCameraViewMatrix();
		}
		
		if(this->_parent_size.x == 0.0f && this->_parent_size.y == 0.0f && this->_parent_size.z == 0.0f)
		{
			this->_parent_size = glm::vec3(1,1,1);
		}

		glm::quat ModelRotation = glm::quat(glm::radians(this->_rotation));
		glm::quat ParentRotation = glm::quat(glm::radians(this->_parent_rotation));
		glm::mat4 Model = 
						glm::scale(
							glm::translate(
								glm::scale(
									glm::translate(glm::mat4(1.0f), 
									this->_parent_pos)
										*
									glm::toMat4(
									ParentRotation),
								this->_parent_size),
							this->_pos)
								*
							glm::toMat4(
							ModelRotation),
						this->_size);

		this->matM = Model;
		this->matV = View;
		this->matMV = View * Model;
		this->matMV3x3 = glm::mat3(View * Model);
		this->matMVP = Projection * View * Model;

		if (!this->has_updated_meshdata)
		{
			this->has_updated_meshdata = true;

			if (this->vbo != nullptr)
			{
				vkDeviceWaitIdle(renderer->GetLogicalDevice());
				renderer->cleanupVulkanBuffer(this->vbo);
				this->vbo = nullptr;
			}

			std::vector<RenderingVertex> generated_mesh = {};

			generated_mesh.resize(this->mesh->mesh_vertices.size());

			uint32_t i = 0;
			for (glm::vec3 pos : this->mesh->mesh_vertices)
			{
				generated_mesh[i].pos = pos;
				generated_mesh[i].color = glm::vec3(0);
				i++;
			}

			i = 0;
			for (glm::vec2 texCoord: this->mesh->mesh_uvs)
			{
				generated_mesh[i].texcoord = texCoord;
				i++;
			}

			i = 0;
			for (glm::vec3 normal : this->mesh->mesh_normals)
			{
				generated_mesh[i].normal = normal;
				i++;
			}

			this->vbo = renderer->createVulkanVertexBufferFromData(generated_mesh);

			this->vbo_vert_count = generated_mesh.size();
			
			std::vector<VkDescriptorImageInfo> diffuseImageBufferInfos{};
			diffuseImageBufferInfos.resize(this->mesh->diffuse_textures.size());
			for (int diffuseTextureIndex = 0; diffuseTextureIndex < this->mesh->diffuse_textures.size(); diffuseTextureIndex++)
			{
				Logging::GlobalLogger->SimpleLog(Logging::LogLevel::Debug2, "Filling imageBufferInfos at index %u", diffuseTextureIndex);

				if(this->mesh->diffuse_textures[diffuseTextureIndex] != nullptr)
				{
					VulkanTextureImage* currentDiffuseTextureImage = this->mesh->diffuse_textures[diffuseTextureIndex]->GetTextureImage();
	
					if(currentDiffuseTextureImage != nullptr)
					{
						if (currentDiffuseTextureImage->image != nullptr || currentDiffuseTextureImage->textureSampler != VK_NULL_HANDLE)
						{
							VkDescriptorImageInfo diffuseImageBufferInfo{};
							diffuseImageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
							diffuseImageBufferInfo.imageView = currentDiffuseTextureImage->image->imageView;
							diffuseImageBufferInfo.sampler = currentDiffuseTextureImage->textureSampler;
							diffuseImageBufferInfos[diffuseTextureIndex] = diffuseImageBufferInfo;
						}
					}
				}
			}

			for (size_t i = 0; i < renderer->GetMaxFramesInFlight(); i++)
			{
				VkDescriptorBufferInfo uniformBufferInfo{};
				uniformBufferInfo.buffer = pipeline->uniformBuffers[i]->buffer;
				uniformBufferInfo.offset = 0;
				uniformBufferInfo.range = sizeof(glm::mat4);

				std::vector<VkWriteDescriptorSet> descriptorWrites{};
				descriptorWrites.resize(1);

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = pipeline->vkDescriptorSets[i];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

				if (diffuseImageBufferInfos.size() > 0)
				{
					Logging::GlobalLogger->SimpleLog(Logging::LogLevel::Debug3, "Updating set 0x%x binding 1", pipeline->vkDescriptorSets[i]);

					descriptorWrites.resize(2);
					descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[1].dstSet = pipeline->vkDescriptorSets[i];
					descriptorWrites[1].dstBinding = 1;
					descriptorWrites[1].dstArrayElement = 0;
					descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrites[1].descriptorCount = static_cast<uint32_t>(diffuseImageBufferInfos.size());
					descriptorWrites[1].pImageInfo = diffuseImageBufferInfos.data();
				}

				Logging::GlobalLogger->SimpleLog(Logging::LogLevel::Debug3, "Descriptor set 0x%x write of index 1 has binding %u, descriptorWrite size is %u",
					pipeline->vkDescriptorSets[i], descriptorWrites[1].dstBinding, static_cast<unsigned int>(descriptorWrites.size()));

				vkUpdateDescriptorSets(renderer->GetLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}

		//
		//	// Vertices, UVs and Normals
		//	glNamedBufferData(this->vbo,
		//		this->mesh->mesh_vertices.size() * sizeof(glm::vec3) + this->mesh->mesh_uvs.size() * sizeof(glm::vec2) + this->mesh->mesh_normals.size() * sizeof(glm::vec3),
		//		NULL, GL_STATIC_DRAW
		//	);

		//	glNamedBufferSubData(this->vbo, 
		//		0,
		//		this->mesh->mesh_vertices.size() * sizeof(glm::vec3),
		//		(void*)&(this->mesh->mesh_vertices[0])
		//	);

		//	glNamedBufferSubData(this->vbo,
		//		this->mesh->mesh_vertices.size() * sizeof(glm::vec3),
		//		this->mesh->mesh_uvs.size() * sizeof(glm::vec2),
		//		(void*)&(this->mesh->mesh_uvs[0])
		//	);

		//	glNamedBufferSubData(this->vbo,
		//		this->mesh->mesh_vertices.size() * sizeof(glm::vec3) + this->mesh->mesh_uvs.size() * sizeof(glm::vec2),
		//		this->mesh->mesh_normals.size() * sizeof(glm::vec3),
		//		(void*)&(this->mesh->mesh_normals[0])
		//	);

		//	// Materials
		//	glNamedBufferData(this->mbo,
		//			this->mesh->mesh_ambient.size() * sizeof(glm::vec3) +
		//			this->mesh->mesh_diffuse.size() * sizeof(glm::vec3) +
		//			this->mesh->mesh_specular.size() * sizeof(glm::vec3) +
		//			this->mesh->matids.size() * sizeof(GLuint) +
		//			this->mesh->mesh_dissolve.size() * sizeof(float) +
		//			this->mesh->mesh_emission.size() * sizeof(glm::vec3),
		//		NULL, GL_STATIC_DRAW
		//	);

		//	size_t offset = 0;
		//	glNamedBufferSubData(this->mbo,
		//		offset,
		//		this->mesh->mesh_ambient.size() * sizeof(glm::vec3),
		//		(void*)&(this->mesh->mesh_ambient[0])
		//	);
		//	offset += this->mesh->mesh_ambient.size() * sizeof(glm::vec3);

		//	glNamedBufferSubData(this->mbo,
		//		offset,
		//		this->mesh->mesh_diffuse.size() * sizeof(glm::vec3),
		//		(void*)&(this->mesh->mesh_diffuse[0])
		//	);
		//	offset += this->mesh->mesh_diffuse.size() * sizeof(glm::vec3);

		//	glNamedBufferSubData(this->mbo,
		//		offset,
		//		this->mesh->mesh_specular.size() * sizeof(glm::vec3),
		//		(void*)&(this->mesh->mesh_specular[0])
		//	);
		//	offset += this->mesh->mesh_specular.size() * sizeof(glm::vec3);

		//	glNamedBufferSubData(this->mbo,
		//		offset,
		//		this->mesh->matids.size() * sizeof(GLuint),
		//		(void*)&(this->mesh->matids[0])
		//	);
		//	offset += this->mesh->matids.size() * sizeof(GLuint);
		//	
		//	glNamedBufferSubData(this->mbo,
		//		offset,
		//		this->mesh->mesh_dissolve.size() * sizeof(float),
		//		(void*)&(this->mesh->mesh_dissolve[0])
		//	);
		//	offset += this->mesh->mesh_dissolve.size() * sizeof(float);

		//	glNamedBufferSubData(this->mbo,
		//		offset,
		//		this->mesh->mesh_emission.size() * sizeof(glm::vec3),
		//		(void*)&(this->mesh->mesh_emission[0])
		//	);

		//	// Tangents and bitangents
		//	glNamedBufferData(this->tbbo,
		//		this->mesh->mesh_tangents.size() * sizeof(glm::vec3) +
		//		this->mesh->mesh_bitangents.size() * sizeof(glm::vec3),
		//		NULL, GL_STATIC_DRAW
		//	);

		//	glNamedBufferSubData(this->tbbo,
		//		0,
		//		this->mesh->mesh_tangents.size() * sizeof(glm::vec3),
		//		(void*)&(this->mesh->mesh_tangents[0])
		//	);

		//	glNamedBufferSubData(this->tbbo,
		//		this->mesh->mesh_tangents.size() * sizeof(glm::vec3),
		//		this->mesh->mesh_bitangents.size() * sizeof(glm::vec3),
		//		(void*)&(this->mesh->mesh_bitangents[0])
		//	);

		//	//Logging::GlobalLogger->SimpleLog(Logging::LogLevel::Info, "%u %u %u %u %u", this->mesh->mesh_vertices.size(), this->mesh->mesh_diffuse.size(), this->mesh->mesh_specular.size(), this->mesh->matids.size(), this->mesh->mesh_dissolve.size());
		//

		//	glBindVertexArray(this->vao);
		//	// Bind vbo and vertex data
		//	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
		//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr); // vertices
		//	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(this->mesh->mesh_vertices.size() * sizeof(glm::vec3))); // uvs
		//	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)(this->mesh->mesh_vertices.size() * sizeof(glm::vec3) + this->mesh->mesh_uvs.size() * sizeof(glm::vec2))); // normals

		//	// Bind mbo and material data
		//	glBindBuffer(GL_ARRAY_BUFFER, this->mbo);
		//	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr); // material ambient
		//	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)(this->mesh->mesh_ambient.size() * sizeof(glm::vec3))); // material diffuse
		//	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 0, (void*)(this->mesh->mesh_ambient.size() * sizeof(glm::vec3) + this->mesh->mesh_diffuse.size() * sizeof(glm::vec3))); // material specular
		//	glVertexAttribIPointer(6, 1, GL_UNSIGNED_INT, 0, (void*)(this->mesh->mesh_ambient.size() * sizeof(glm::vec3) + this->mesh->mesh_diffuse.size() * sizeof(glm::vec3) + this->mesh->mesh_specular.size() * sizeof(glm::vec3))); // material ids
		//	glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, 0, (void*)(this->mesh->mesh_ambient.size() * sizeof(glm::vec3) + this->mesh->mesh_diffuse.size() * sizeof(glm::vec3) + this->mesh->mesh_specular.size() * sizeof(glm::vec3) + this->mesh->matids.size() * sizeof(GLuint))); // material dissolve
		//	glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, 0, (void*)(this->mesh->mesh_ambient.size() * sizeof(glm::vec3) + this->mesh->mesh_diffuse.size() * sizeof(glm::vec3) + this->mesh->mesh_specular.size() * sizeof(glm::vec3) + this->mesh->matids.size() * sizeof(GLuint) + this->mesh->mesh_dissolve.size() * sizeof(float))); // material emission

		//	// Bind tbbo and tangent/bitangent data
		//	glBindBuffer(GL_ARRAY_BUFFER, this->tbbo);
		//	glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, 0, nullptr); // material ambient
		//	glVertexAttribPointer(10, 3, GL_FLOAT, GL_FALSE, 0, (void*)(this->mesh->mesh_tangents.size() * sizeof(glm::vec3))); // material diffuse

		}
	}

	void MeshRenderer::Render(VkCommandBuffer commandBuffer, uint32_t currentFrame)
	{
		if (!this->has_updated_mesh)
		{
			this->UpdateMesh();
		}

		// If no shader bound, throw exception
		/*if (!this->program)
		{
			Logging::GlobalLogger->SimpleLog(Logging::LogLevel::Exception, "No program assigned to MeshRenderer, cannot perform Engine::Rendering::MeshRenderer::Render()");
			exit(1);
		}*/

		VulkanRenderingEngine* renderer = this->owner_engine->GetRenderingEngine();
		vkMapMemory(renderer->GetLogicalDevice(),
			pipeline->uniformBuffers[currentFrame]->allocationInfo.deviceMemory,
			pipeline->uniformBuffers[currentFrame]->allocationInfo.offset,
			pipeline->uniformBuffers[currentFrame]->allocationInfo.size,
			0,
			&(pipeline->uniformBuffers[currentFrame]->mappedData));

		memcpy(this->pipeline->uniformBuffers[currentFrame]->mappedData, &this->matMVP, sizeof(glm::mat4));
		vkUnmapMemory(renderer->GetLogicalDevice(), pipeline->uniformBuffers[currentFrame]->allocationInfo.deviceMemory);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline->vkPipelineLayout, 0, 1, &this->pipeline->vkDescriptorSets[currentFrame], 0, nullptr);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline->pipeline);
		VkBuffer vertexBuffers[] = { this->vbo->buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdDraw(commandBuffer, static_cast<uint32_t>(this->vbo_vert_count), 1, 0, 0);

		//glBindVertexArray(this->vao);

		//// Bind shader
		//GLuint shader = this->program->GetProgram();

		//if (shader == 0)
		//{
		//	throw std::exception("MeshRenderer: Shader::GetProgram() returned zero! Bad shader!");
		//}

		//glUseProgram(shader);

		//GLint maxTextures;
		//glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextures);

		//// Bind diffuse textures
		//unsigned int textureoffset = 0;
		//for (unsigned int offset = 0; offset < this->mesh->diffuse_textures.size(); offset++)
		//{
		//	if (this->mesh->diffuse_textures[offset] != nullptr)
		//	{
		//		std::shared_ptr<Rendering::Texture> texture = this->mesh->diffuse_textures[offset];

		//		GLuint textureLocation = glGetUniformLocation(shader, std::string("textureDiffuse").c_str());

		//		glUniform1i(textureLocation, offset);
		//		glActiveTexture(GL_TEXTURE0 + textureoffset);
		//		glBindTexture(GL_TEXTURE_2D, texture->GetTexture());

		//		GLuint textureUsedLocation = glGetUniformLocation(shader, std::string("used_textureDiffuse").c_str());
		//		glUniform1i(textureUsedLocation, 1);

		//		if (textureoffset > maxTextures)
		//		{
		//			throw std::exception("MeshRenderer: Texture limit reached! Too many textures!");
		//		}
		//	}
		//	else
		//	{
		//		glActiveTexture(GL_TEXTURE0 + textureoffset);
		//		glBindTexture(GL_TEXTURE_2D, 0);

		//		GLuint textureUsedLocation = glGetUniformLocation(shader, std::string("used_textureDiffuse").append(std::to_string(offset)).c_str());
		//		glUniform1i(textureUsedLocation, 0);
		//	}
		//	textureoffset++;
		//}

		//for (unsigned int offset = 0; offset < this->mesh->bump_textures.size(); offset++)
		//{
		//	if (this->mesh->bump_textures[offset] != nullptr)
		//	{
		//		std::shared_ptr<Rendering::Texture> texture = this->mesh->bump_textures[offset];

		//		GLuint textureLocation = glGetUniformLocation(shader, std::string("textureNormal").append(std::to_string(offset)).c_str());

		//		glUniform1i(textureLocation, offset);
		//		glActiveTexture(GL_TEXTURE0 + textureoffset);
		//		glBindTexture(GL_TEXTURE_2D, texture->GetTexture());

		//		GLuint textureUsedLocation = glGetUniformLocation(shader, std::string("used_textureNormal").append(std::to_string(offset)).c_str());
		//		glUniform1i(textureUsedLocation, 1);

		//		if (textureoffset > maxTextures)
		//		{
		//			throw std::exception("MeshRenderer: Texture limit reached! Too many textures!");
		//		}
		//	}
		//	else
		//	{
		//		glActiveTexture(GL_TEXTURE0 + textureoffset);
		//		glBindTexture(GL_TEXTURE_2D, 0);

		//		GLuint textureUsedLocation = glGetUniformLocation(shader, std::string("used_textureNormal").append(std::to_string(offset)).c_str());
		//		glUniform1i(textureUsedLocation, 0);
		//	}
		//	textureoffset++;
		//}
		//
		//// Bind matrixes
		//GLuint m_uniform_id = glGetUniformLocation(shader, "matM");
		//glUniformMatrix4fv(m_uniform_id, 1, GL_FALSE, &this->matM[0][0]);

		//GLuint v_uniform_id = glGetUniformLocation(shader, "matV");
		//glUniformMatrix4fv(v_uniform_id, 1, GL_FALSE, &this->matV[0][0]);

		//GLuint mv_uniform_id = glGetUniformLocation(shader, "matMV");
		//glUniformMatrix4fv(mv_uniform_id, 1, GL_FALSE, &this->matMV[0][0]);

		//GLuint mv3x3_uniform_id = glGetUniformLocation(shader, "matMV3x3");
		//glUniformMatrix3fv(mv3x3_uniform_id, 1, GL_FALSE, &this->matMV3x3[0][0]);

		//GLuint mvp_uniform_id = glGetUniformLocation(shader, "matMVP");
		//glUniformMatrix4fv(mvp_uniform_id, 1, GL_FALSE, &this->matMVP[0][0]);

		//GLuint light_uniform_id = glGetUniformLocation(shader, "Light");
		//glm::vec3 light_position = global_scene_manager->GetLightTransform();
		//glUniform3f(light_uniform_id, light_position.x, light_position.y, light_position.z);

		//// Enable vertex attribs
		//for (int i = 0; i < 11; i++)
		//{
		//	glEnableVertexAttribArray(i);
		//}

		//glDrawArrays(GL_TRIANGLES, 0, GLsizei(this->mesh->mesh_vertices.size()));

		//// Disable vertex attribs
		//for (int i = 0; i < 11; i++)
		//{
		//	glDisableVertexAttribArray(i);
		//}

		//// Unbind textures
		//for (int i = 0; i < textureoffset; i++)
		//{
		//	glActiveTexture(GL_TEXTURE0 + i);
		//	glBindTexture(GL_TEXTURE_2D, 0);
		//}
	}
}