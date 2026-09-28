/**
 * @file script_system.cpp
 * @brief Implementation of the ScriptSystem for external script management.
 */
#define NOMINMAX
#include "ecs/systems/script_system.h"
#include "ecs/components/script.h"
#include "core/log.h"
#include <cstring>
#include <sstream>
#include <algorithm>

#include <windows.h>

namespace Dnd {

ScriptSystem::ScriptSystem()
    : System("ScriptSystem", SystemPhase::UPDATE, 200) {
}

ScriptSystem::~ScriptSystem() {
}

std::string ScriptSystem::BuildCommand(const ScriptComponent& script) {
    std::string cmd;
    switch (script.language) {
        case ScriptLanguage::Python:
            cmd = "python ";
            break;
        case ScriptLanguage::NodeJS:
            cmd = "node ";
            break;
        default:
            return "";
    }
    cmd += "\"";
    cmd += script.scriptPath;
    cmd += "\"";
    return cmd;
}

b8 ScriptSystem::StartScript(ComponentRegistry& registry, u32 entityId) {
    Entity e(Entity::EncodeId(entityId, 0));
    ScriptComponent* script = registry.GetComponent<ScriptComponent>(e);
    if (!script || script->isRunning) return false;

    std::string cmd = BuildCommand(*script);
    if (cmd.empty()) {
        DND_LOG_WARN("ScriptSystem: no command for entity %u", entityId);
        return false;
    }

    DND_LOG_INFO("ScriptSystem: launching '%s' for entity %u", cmd.c_str(), entityId);

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE childStdinRd, childStdinWr;
    HANDLE childStdoutRd, childStdoutWr;

    if (!CreatePipe(&childStdinRd, &childStdinWr, &sa, 0)) {
        DND_LOG_ERROR("ScriptSystem: failed to create stdin pipe");
        return false;
    }
    if (!SetHandleInformation(childStdinWr, HANDLE_FLAG_INHERIT, 0)) {
        DND_LOG_ERROR("ScriptSystem: failed to set stdin handle info");
        CloseHandle(childStdinRd); CloseHandle(childStdinWr);
        return false;
    }

    if (!CreatePipe(&childStdoutRd, &childStdoutWr, &sa, 0)) {
        DND_LOG_ERROR("ScriptSystem: failed to create stdout pipe");
        CloseHandle(childStdinRd); CloseHandle(childStdinWr);
        return false;
    }
    if (!SetHandleInformation(childStdoutRd, HANDLE_FLAG_INHERIT, 0)) {
        DND_LOG_ERROR("ScriptSystem: failed to set stdout handle info");
        CloseHandle(childStdinRd); CloseHandle(childStdinWr);
        CloseHandle(childStdoutRd); CloseHandle(childStdoutWr);
        return false;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdInput = childStdinRd;
    si.hStdOutput = childStdoutWr;
    si.hStdError = childStdoutWr;
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(pi));

    char cmdBuf[1024];
    strncpy_s(cmdBuf, cmd.c_str(), sizeof(cmdBuf) - 1);

    BOOL success = CreateProcessA(
        nullptr, cmdBuf, nullptr, nullptr, TRUE, 0,
        nullptr, nullptr, &si, &pi
    );

    CloseHandle(childStdinRd);
    CloseHandle(childStdoutWr);

    if (!success) {
        DND_LOG_ERROR("ScriptSystem: failed to launch process (error %lu)", GetLastError());
        CloseHandle(childStdoutRd); CloseHandle(childStdinWr);
        return false;
    }

    script->pipeWrite = childStdinWr;
    script->pipeRead = childStdoutRd;
    script->processHandle = pi.hProcess;
    script->isRunning = true;
    script->readLen = 0;
    script->pollCounter = 0;

    CloseHandle(pi.hThread);
    DND_LOG_INFO("ScriptSystem: process launched (PID %lu)", pi.dwProcessId);
    return true;
}

void ScriptSystem::StopScript(ComponentRegistry& registry, u32 entityId) {
    Entity e(Entity::EncodeId(entityId, 0));
    ScriptComponent* script = registry.GetComponent<ScriptComponent>(e);
    if (!script || !script->isRunning) return;

    DND_LOG_DEBUG("ScriptSystem: stopping script for entity %u", entityId);

    if (script->processHandle) {
        TerminateProcess(script->processHandle, 0);
        WaitForSingleObject(script->processHandle, 1000);
        CloseHandle(script->processHandle);
        script->processHandle = nullptr;
    }
    if (script->pipeWrite) {
        CloseHandle(script->pipeWrite);
        script->pipeWrite = nullptr;
    }
    if (script->pipeRead) {
        CloseHandle(script->pipeRead);
        script->pipeRead = nullptr;
    }
    script->isRunning = false;
    script->awaitingResponse = false;
    script->readLen = 0;
}

void ScriptSystem::ScriptSendMessage(ScriptComponent* script, const std::string& message) {
    if (!script || !script->isRunning || !script->pipeWrite) return;

    std::string payload = message + "\n";
    DWORD written;
    WriteFile(script->pipeWrite, payload.c_str(), static_cast<DWORD>(payload.size()), &written, nullptr);
}

std::string ScriptSystem::ReadResponse(ScriptComponent* script) {
    if (!script || !script->isRunning || !script->pipeRead) return "";

    DWORD available = 0;
    if (!PeekNamedPipe(script->pipeRead, nullptr, 0, nullptr, &available, nullptr)) {
        return "";
    }
    if (available == 0) return "";

    DWORD read = 0;
    DWORD toRead = (std::min)(available, static_cast<DWORD>(sizeof(script->readBuffer) - script->readLen - 1));
    if (toRead == 0) return "";

    if (!ReadFile(script->pipeRead, script->readBuffer + script->readLen, toRead, &read, nullptr)) {
        return "";
    }
    script->readLen += static_cast<i32>(read);
    script->readBuffer[script->readLen] = '\0';

    std::string result;
    char* buf = script->readBuffer;
    i32 bufLen = script->readLen;

    i32 lineStart = 0;
    for (i32 i = 0; i < bufLen; ++i) {
        if (buf[i] == '\n') {
            buf[i] = '\0';
            result += (buf + lineStart);
            result += '\n';
            lineStart = i + 1;
        }
    }

    if (lineStart < bufLen) {
        i32 remaining = bufLen - lineStart;
        memmove(buf, buf + lineStart, remaining);
        script->readLen = remaining;
    } else {
        script->readLen = 0;
    }

    return result;
}

void ScriptSystem::OnUpdate(f32 deltaTime, EntityManager& em, ComponentRegistry& registry) {
    (void)deltaTime;

    SparseSet<ScriptComponent>* scripts = registry.GetSparseSet<ScriptComponent>();
    if (!scripts) return;

    for (u32 i = 0; i < scripts->Size(); ++i) {
        u32 entityIdx = scripts->GetEntityIndex(i);

        Entity e(Entity::EncodeId(entityIdx, em.GetGeneration(entityIdx)));
        if (!em.IsValid(e)) continue;

        ScriptComponent* script = registry.GetComponent<ScriptComponent>(e);
        if (!script) continue;

        if (!script->isRunning && script->autoStart && script->language != ScriptLanguage::None) {
            StartScript(registry, entityIdx);
        }

        if (script->isRunning) {
            script->pollCounter++;
            if (script->pollCounter >= POLL_INTERVAL) {
                script->pollCounter = 0;
                std::string response = ReadResponse(script);
                if (!response.empty()) {
                    DND_LOG_DEBUG("ScriptSystem: response from entity %u: %s",
                                  entityIdx, response.c_str());
                }
            }

            if (script->processHandle) {
                DWORD exitCode = 0;
                if (GetExitCodeProcess(script->processHandle, &exitCode)) {
                    if (exitCode != STILL_ACTIVE) {
                        DND_LOG_WARN("ScriptSystem: script for entity %u exited with code %lu",
                                     entityIdx, exitCode);
                        StopScript(registry, entityIdx);
                    }
                }
            }
        }
    }
}

void ScriptSystem::NotifyEvent(ComponentRegistry& registry, Entity entity, const std::string& eventJson) {
    ScriptComponent* script = registry.GetComponent<ScriptComponent>(entity);
    if (script && script->isRunning) {
        ScriptSendMessage(script, eventJson);
    }
}

void ScriptSystem::StopAll(ComponentRegistry& registry) {
    SparseSet<ScriptComponent>* scripts = registry.GetSparseSet<ScriptComponent>();
    if (!scripts) return;

    for (u32 i = 0; i < scripts->Size(); ++i) {
        u32 entityIdx = scripts->GetEntityIndex(i);
        StopScript(registry, entityIdx);
    }
}

} // namespace Dnd
