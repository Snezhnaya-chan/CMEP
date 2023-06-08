#pragma once

#include <functional>
#include <optional>

#include "Rendering/IRenderer.hpp"

namespace Engine
{
	class __declspec(dllexport) Object
	{
	protected:
		/// <summary>
		/// Position of object in worldspace.
		/// </summary>
		glm::vec3 _pos = glm::vec3();

		/// <summary>
		/// Size of object.
		/// </summary>
		glm::vec3 _size = glm::vec3();

		/// <summary>
		/// Rotation of object.
		/// </summary>
		glm::vec3 _rotation = glm::vec3();

		unsigned int screenx = 0, screeny = 0;

		std::function<void(Object*)> _onClick = nullptr;

	public:
		Rendering::IRenderer* renderer = nullptr;
		
		Object() noexcept {}

		void ScreenSizeInform(unsigned int screenx, unsigned int screeny) noexcept
		{ 
			this->screenx = screenx;
			this->screeny = screeny;
			if (this->renderer != nullptr) { this->renderer->Update(this->_pos, this->_size, this->_rotation, this->screenx, this->screeny); }
		}

		virtual void Translate(const glm::vec3 pos) noexcept
		{
			this->_pos = pos;
			if (this->renderer != nullptr) { this->renderer->Update(this->_pos, this->_size, this->_rotation, this->screenx, this->screeny); }
		}

		virtual void Scale(const glm::vec3 size) noexcept
		{ 
			this->_size = size; 
			if (this->renderer != nullptr) { this->renderer->Update(this->_pos, this->_size, this->_rotation, this->screenx, this->screeny); }
		}

		virtual void Rotate(const glm::vec3 rotation) noexcept 
		{ 
			this->_rotation = rotation;
			if (this->renderer != nullptr) { this->renderer->Update(this->_pos, this->_size, this->_rotation, this->screenx, this->screeny); }
		}

		virtual int Render() 
		{ 
			if (this->renderer != nullptr) { this->renderer->Render(); } 
			return 0; 
		}

		void RegisterOnClick(std::function<void(Object*)> f) noexcept { this->_onClick = f; };
		void onClick()
		{ 
			try { this->_onClick(this); } 
			// std::bad_function_call is thrown when this->_onClick has no function assigned, ignore
			catch(std::bad_function_call e) { /* exception ignored */ }
		}

		glm::vec3 position() const noexcept { return this->_pos; }
		glm::vec3 size() const noexcept { return this->_size; }
		glm::vec3 rotation() const noexcept { return this->_rotation; }

	};
}