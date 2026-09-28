/**
 * @file ai_brain.cpp
 * @brief Implementation of the AiBrain LLM bridge (Ollama via curl).
 *
 * See ai_brain.h. The request runs as a background curl.exe process; Poll()
 * checks (non-blocking) whether it has exited, then reads + parses the reply.
 *
 * @author DND Engine Team
 * @date 2026-06-03
 * @version 0.11.0
 */

#include "ai/ai_brain.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cctype>

namespace Dnd {

namespace {

b8     s_busy = false;
HANDLE s_proc = nullptr;

const char* kReqPath  = "ai_req.json";
const char* kRespPath = "ai_resp.json";

/// @brief Escapes a string for embedding inside a JSON string literal.
std::string JsonEscape(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:   o += c;      break;
        }
    }
    return o;
}

/// @brief Reads an entire file into a string (empty if missing).
std::string ReadFileStr(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return std::string();
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

/// @brief Extracts the value of the first  "key":"..."  string from JSON,
///        un-escaping \n \t \r \" \\. Returns empty if not found.
std::string ExtractJsonString(const std::string& json, const char* key) {
    const std::string pat = std::string("\"") + key + "\"";
    size_t k = json.find(pat);
    if (k == std::string::npos) return std::string();
    size_t colon = json.find(':', k + pat.size());
    if (colon == std::string::npos) return std::string();
    size_t q = json.find('"', colon + 1);
    if (q == std::string::npos) return std::string();
    std::string out;
    for (size_t i = q + 1; i < json.size(); ++i) {
        char c = json[i];
        if (c == '\\' && i + 1 < json.size()) {
            char n = json[++i];
            switch (n) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case '"': out += '"';  break;
                case '\\': out += '\\'; break;
                default:  out += n;    break;
            }
            continue;
        }
        if (c == '"') break;
        out += c;
    }
    return out;
}

/// @brief Returns the text after "KEY:" on the line that starts with it
///        (case-insensitive), trimmed. Empty if no such line.
std::string LineValue(const std::string& content, const char* key) {
    const std::string ukey = key;
    size_t pos = 0;
    while (pos <= content.size()) {
        size_t eol = content.find('\n', pos);
        std::string line = content.substr(pos, eol == std::string::npos ? std::string::npos : eol - pos);
        size_t s = line.find_first_not_of(" \t");
        if (s != std::string::npos) {
            const std::string t = line.substr(s);
            bool match = t.size() >= ukey.size();
            for (size_t j = 0; match && j < ukey.size(); ++j) {
                if (std::toupper((unsigned char)t[j]) != std::toupper((unsigned char)ukey[j])) match = false;
            }
            if (match) {
                std::string v = t.substr(ukey.size());
                size_t vs = v.find_first_not_of(" \t");
                v = (vs == std::string::npos) ? std::string() : v.substr(vs);
                while (!v.empty() && (v.back() == '\r' || v.back() == '\n' || v.back() == ' ')) v.pop_back();
                return v;
            }
        }
        if (eol == std::string::npos) break;
        pos = eol + 1;
    }
    return std::string();
}

} // namespace

b8 AiBrain::Submit(const std::string& systemPrompt, const std::string& userText, const char* model) {
    if (s_busy) return false;

    // Build the Ollama /api/chat request body.
    std::string body = std::string("{\"model\":\"") + model + "\",\"stream\":false,\"messages\":[";
    body += "{\"role\":\"system\",\"content\":\"" + JsonEscape(systemPrompt) + "\"},";
    body += "{\"role\":\"user\",\"content\":\"" + JsonEscape(userText) + "\"}]}";

    {
        std::ofstream f(kReqPath, std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f << body;
    }
    DeleteFileA(kRespPath);

    // curl.exe POSTs the body to the local Ollama server and writes the reply.
    std::string cmd =
        std::string("curl.exe -s -o \"") + kRespPath +
        "\" -X POST http://localhost:11434/api/chat "
        "-H \"Content-Type: application/json\" --data-binary @\"" + kReqPath + "\"";

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    std::vector<char> cmdbuf(cmd.begin(), cmd.end());
    cmdbuf.push_back('\0');

    if (!CreateProcessA(nullptr, cmdbuf.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }
    CloseHandle(pi.hThread);
    s_proc = pi.hProcess;
    s_busy = true;
    return true;
}

b8 AiBrain::IsBusy() { return s_busy; }

b8 AiBrain::Poll(AiAction& out) {
    if (!s_busy) return false;
    if (WaitForSingleObject(s_proc, 0) != WAIT_OBJECT_0) return false; // still running

    CloseHandle(s_proc);
    s_proc = nullptr;
    s_busy = false;

    out = AiAction();
    const std::string resp = ReadFileStr(kRespPath);
    if (resp.empty()) {
        out.error = "no response (is ollama serving on :11434?)";
        return true;
    }
    const std::string content = ExtractJsonString(resp, "content");
    if (content.empty()) {
        const std::string err = ExtractJsonString(resp, "error");
        out.error = err.empty() ? "could not parse model reply" : err;
        out.raw = resp;
        return true;
    }

    out.raw       = content;
    out.action    = LineValue(content, "ACTION:");
    out.target    = LineValue(content, "TARGET:");
    out.ability   = LineValue(content, "ABILITY:");
    out.skill     = LineValue(content, "SKILL:");
    out.narration = LineValue(content, "NARRATION:");
    const std::string dcs = LineValue(content, "DC:");
    if (!dcs.empty()) out.dc = std::atoi(dcs.c_str());

    out.ok = !out.action.empty();
    if (!out.ok) out.error = "reply had no ACTION line";
    return true;
}

void AiBrain::Shutdown() {
    if (s_proc) {
        TerminateProcess(s_proc, 0);
        CloseHandle(s_proc);
        s_proc = nullptr;
    }
    s_busy = false;
}

} // namespace Dnd
