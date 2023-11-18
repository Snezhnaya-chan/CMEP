#pragma once

#include "Object.hpp"
#include "InternalEngineObject.hpp"
#include "PlatformSemantics.hpp"
#include "EventHandling.hpp"
#include "Scripting/LuaScript.hpp"

#include <map>

#include <unordered_map>

namespace Engine
{
    class Engine;

    class CMEP_EXPORT Scene : public InternalEngineObject
    {
    private:
    protected:
        std::unordered_map<std::string, Object*> objects{};
        std::unordered_map<std::string, Object*> templates{};

    public:
		std::multimap<EventHandling::EventType, std::pair<std::shared_ptr<Scripting::LuaScript>, std::string>> lua_event_handlers;

        Scene();
        ~Scene();

		const std::unordered_map<std::string, Object*>* const GetAllObjects() noexcept;

		void AddObject(std::string name, Object* ptr);
		Object* AddTemplatedObject(std::string name, std::string template_name);
		Object* FindObject(std::string name);
		size_t RemoveObject(std::string name) noexcept;

        inline void LoadTemplatedObject(std::string name, Object* object) { this->templates.emplace(name, object); }
    };
}