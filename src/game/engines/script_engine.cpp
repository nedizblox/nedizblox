#include "script_engine.hpp"

#include <wren/wren_opt_random.h>

#include <cstring>

namespace game::engines {

ScriptEngine::ScriptEngine() {
    init();
}

ScriptEngine::~ScriptEngine() {
    wrenFreeVM(m_vm);
}

void ScriptEngine::init() {
    WrenConfiguration config;
    wrenInitConfiguration(&config);

    config.bindForeignMethodFn = &bindForeignMethod;
    config.bindForeignClassFn = &bindForeignClass;

    config.writeFn = &writeLog;

    m_vm = wrenNewVM(&config);

    wrenSetUserData(m_vm, this);
}

void ScriptEngine::setupEnviroment(const std::string& module) {
    //const char* randomSource = wrenRandomSource();

    //wrenInterpret(m_vm, module.c_str(), randomSource);
}

WrenForeignMethodFn ScriptEngine::bindForeignMethod(WrenVM* vm, const char* module, const char* className, bool isStatic, const char* signature) {
    if (std::strcmp(className, "Random") == 0) {
        //return wrenRandomBindForeignMethod(vm, className, isStatic, signature);
    }

    return nullptr;
}

WrenForeignClassMethods ScriptEngine::bindForeignClass(WrenVM* vm, const char* module, const char* className) {
    WrenForeignClassMethods methods = {nullptr, nullptr};

    if (std::strcmp(className, "Random") == 0) {
        //return wrenRandomBindForeignClass(vm, module, className);
    }

    return methods;
}

void ScriptEngine::writeLog(WrenVM* vm, const char* text) {
    ScriptEngine* self = static_cast<ScriptEngine*>(wrenGetUserData(vm));

    self->m_logBuffer.push_back(text);
}

void ScriptEngine::runString(const std::string& name, const std::string& source) {
    setupEnviroment(name);

    wrenInterpret(m_vm, name.c_str(), source.c_str());
}

} // namespace game::engines