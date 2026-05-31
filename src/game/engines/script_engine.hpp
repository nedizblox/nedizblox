#pragma once

#include <lua.h>
#include <luacode.h>
#include <lualib.h>

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace game::engines {

class ScriptEngine {
public:
    using LuaCallback = std::function<int(lua_State*)>;

    ScriptEngine();
    ~ScriptEngine();

    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

    const std::vector<std::string>& getLogBuffer() const { return m_logBuffer; }

    void update();

    void clearLog();

    void createMetatable(const std::string& name, const std::unordered_map<std::string, LuaCallback>& methods);
    void createFunction(const std::string& name, LuaCallback func);

    template <typename T>
    T* createUserdata(const std::string& metaName) {
        T* data = static_cast<T*>(lua_newuserdata(m_lua, sizeof(T)));
        new (data) T();

        luaL_getmetatable(m_lua, metaName.c_str());

        if (lua_isnil(m_lua, -1)) {
            lua_pop(m_lua, 2);
            return nullptr;
        }

        lua_setmetatable(m_lua, -2);

        return data;
    }

    void createTable(const std::string& name);

    template <typename T>
    void addItemToTable(const std::string& tableName, const std::string& metaName, const std::string& key, T* item) {
        lua_getglobal(m_lua, tableName.c_str());

        if (lua_istable(m_lua, -1)) {
            T** udata = static_cast<T**>(lua_newuserdata(m_lua, sizeof(T*)));
            *udata = item;

            luaL_getmetatable(m_lua, metaName.c_str());
            lua_setmetatable(m_lua, -2);

            lua_setfield(m_lua, -2, key.c_str());
        }

        lua_pop(m_lua, 1);
    }

    void runString(const std::string& chunkName, const std::string& source);

private:
    struct Script {
        lua_State* thread;
        int ref;
    };

    lua_State* m_lua;
    std::mutex m_luaMutex;

    std::vector<Script> m_activeScripts;
    std::vector<std::string> m_logBuffer;

    void setupEnviroment();
};

} // namespace game::engines