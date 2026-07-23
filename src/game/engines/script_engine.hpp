#pragma once

#include <wren/wren.hpp>

#include <string>
#include <vector>

namespace game::engines {

class ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();

    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

    const std::vector<std::string>& getLogBuffer() const { return m_logBuffer; }

    void clearLog() { m_logBuffer.clear(); }

    void runString(const std::string& name, const std::string& source);

private:
    WrenVM* m_vm = nullptr;

    std::vector<std::string> m_logBuffer;

    void init();
    void setupEnviroment(const std::string& module);

    static WrenForeignMethodFn
    bindForeignMethod(WrenVM* vm, const char* module, const char* className, bool isStatic, const char* signature);
    static WrenForeignClassMethods bindForeignClass(WrenVM* vm, const char* module, const char* className);

    static void writeLog(WrenVM* vm, const char* text);
};

} // namespace game::engines