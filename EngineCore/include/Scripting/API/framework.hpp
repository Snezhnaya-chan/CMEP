#pragma once

#include <unordered_map>
#include <memory>

#include "Scripting/lualib/lua.hpp"

#define CMEP_LUAMAPPING_DEFINE(namespace, mapping) {#mapping, namespace::mapping }
