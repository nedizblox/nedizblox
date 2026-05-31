#include "script_engine.hpp"

#include <Luau/Compiler.h>

#include <format>

namespace game::engines {

ScriptEngine::ScriptEngine() {
    m_lua = luaL_newstate();
    luaL_openlibs(m_lua);

    setupEnviroment();
}

ScriptEngine::~ScriptEngine() { lua_close(m_lua); }

void ScriptEngine::setupEnviroment() {
    createFunction("print", [this](lua_State* L) -> int {
        int n = lua_gettop(L);
        std::string line = "";

        for (int i = 1; i <= n; i++) {
            size_t len;
            const char* s = luaL_tolstring(L, i, &len);
            if (i > 1)
                line += "\t";
            line.append(s, len);
            lua_pop(L, 1);
        }

        this->m_logBuffer.push_back(line);

        return 0;
    });

    createFunction("wait", [this](lua_State* L) -> int {
        // double seconds = luaL_optnumber(L, 1, 0.1);
        return lua_yield(L, 1);
    });
}

void ScriptEngine::update() {
    auto it = m_activeScripts.begin();
    while (it != m_activeScripts.end()) {
        int status = lua_resume(it->thread, m_lua, 0);

        if (status == LUA_YIELD) {
            it++;
        } else {
            if (status != LUA_OK) {
                std::string err = lua_tostring(it->thread, -1);
                m_logBuffer.push_back(std::format("Runtime Error: {}", err));

                lua_settop(it->thread, 0);
            }

            lua_unref(m_lua, it->ref);
            it = m_activeScripts.erase(it);
        }
    }
}

void ScriptEngine::clearLog() { m_logBuffer.clear(); }

void ScriptEngine::createTable(const std::string& name) {
    lua_newtable(m_lua);
    lua_setglobal(m_lua, name.c_str());
}

void ScriptEngine::createMetatable(const std::string& name, const std::unordered_map<std::string, LuaCallback>& methods) {
    luaL_newmetatable(m_lua, name.c_str());

    for (const auto& method : methods) {
        void* userData = lua_newuserdata(m_lua, sizeof(LuaCallback));
        new (userData) LuaCallback(method.second);

        lua_pushcclosure(
            m_lua,
            [](lua_State* L) -> int {
                auto* callback = static_cast<LuaCallback*>(lua_touserdata(L, lua_upvalueindex(1)));
                return (*callback)(L);
            },
            name.c_str(), 1);

        lua_setfield(m_lua, -2, method.first.c_str());
    }

    lua_pop(m_lua, 1);
}

void ScriptEngine::createFunction(const std::string& name, LuaCallback func) {
    void* userData = lua_newuserdata(m_lua, sizeof(LuaCallback));
    new (userData) LuaCallback(func);

    lua_pushcclosure(
        m_lua,
        [](lua_State* L) -> int {
            auto* callback = static_cast<LuaCallback*>(lua_touserdata(L, lua_upvalueindex(1)));
            if (callback && *callback) {
                return (*callback)(L);
            }
            return 0;
        },
        name.c_str(), 1);

    lua_setglobal(m_lua, name.c_str());
}

void ScriptEngine::runString(const std::string& chunkName, const std::string& source) {
    std::string bytecode = Luau::compile(source);

    std::lock_guard<std::mutex> lock(m_luaMutex);

    lua_State* thread = lua_newthread(m_lua);

    int threadRef = lua_ref(m_lua, -1);

    if (luau_load(thread, chunkName.c_str(), bytecode.data(), bytecode.size(), 0) == 0) {
        m_activeScripts.push_back({thread, threadRef});
    } else {
        std::string err = lua_tostring(thread, -1);
        m_logBuffer.push_back(err);

        lua_unref(m_lua, threadRef);
    }

    lua_pop(m_lua, 1);
}

} // namespace game::engines