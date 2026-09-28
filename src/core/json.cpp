#include "core/json.h"
#include <cstring>
#include <sstream>
#include <algorithm>

namespace Dnd {

// ==========================================
// JsonValue
// ==========================================

JsonValue::JsonValue() : type(JsonType::Null), boolVal(false), numVal(0.0) {}
JsonValue::JsonValue(std::nullptr_t) : type(JsonType::Null), boolVal(false), numVal(0.0) {}
JsonValue::JsonValue(bool val) : type(JsonType::Bool), boolVal(val), numVal(0.0) {}
JsonValue::JsonValue(double val) : type(JsonType::Number), boolVal(false), numVal(val) {}
JsonValue::JsonValue(int val) : type(JsonType::Number), boolVal(false), numVal(static_cast<double>(val)) {}
JsonValue::JsonValue(const char* val) : type(JsonType::String), boolVal(false), numVal(0.0), strVal(val ? val : "") {}
JsonValue::JsonValue(const std::string& val) : type(JsonType::String), boolVal(false), numVal(0.0), strVal(val) {}

JsonValue JsonValue::Array() { JsonValue v; v.type = JsonType::Array; return v; }
JsonValue JsonValue::Object() { JsonValue v; v.type = JsonType::Object; return v; }

bool JsonValue::IsNull() const { return type == JsonType::Null; }

JsonValue& JsonValue::operator[](const char* key) {
    for (auto& pair : objVal) {
        if (pair.first == key) return pair.second;
    }
    objVal.push_back({key, JsonValue()});
    return objVal.back().second;
}

const JsonValue& JsonValue::operator[](const char* key) const {
    static JsonValue nullValue;
    for (const auto& pair : objVal) {
        if (pair.first == key) return pair.second;
    }
    return nullValue;
}

JsonValue& JsonValue::operator[](size_t idx) {
    if (idx >= arrVal.size()) arrVal.resize(idx + 1);
    return arrVal[idx];
}

const JsonValue& JsonValue::operator[](size_t idx) const {
    static JsonValue nullValue;
    if (idx >= arrVal.size()) return nullValue;
    return arrVal[idx];
}

size_t JsonValue::Size() const {
    switch (type) {
        case JsonType::Array: return arrVal.size();
        case JsonType::Object: return objVal.size();
        default: return 0;
    }
}

bool JsonValue::Has(const char* key) const {
    if (type != JsonType::Object) return false;
    for (const auto& pair : objVal) {
        if (pair.first == key) return true;
    }
    return false;
}

JsonValue* JsonValue::Get(const char* key) {
    if (type != JsonType::Object) return nullptr;
    for (auto& pair : objVal) {
        if (pair.first == key) return &pair.second;
    }
    return nullptr;
}

const JsonValue* JsonValue::Get(const char* key) const {
    if (type != JsonType::Object) return nullptr;
    for (const auto& pair : objVal) {
        if (pair.first == key) return &pair.second;
    }
    return nullptr;
}

double JsonValue::GetNumber(const char* key, double def) const {
    const JsonValue* v = Get(key);
    return v ? v->AsNumber(def) : def;
}

bool JsonValue::GetBool(const char* key, bool def) const {
    const JsonValue* v = Get(key);
    return v ? v->AsBool(def) : def;
}

std::string JsonValue::GetString(const char* key, const std::string& def) const {
    const JsonValue* v = Get(key);
    return v ? v->AsString(def) : def;
}

int JsonValue::GetInt(const char* key, int def) const {
    const JsonValue* v = Get(key);
    return v ? v->AsInt(def) : def;
}

double JsonValue::AsNumber(double def) const {
    return type == JsonType::Number ? numVal : def;
}

bool JsonValue::AsBool(bool def) const {
    return type == JsonType::Bool ? boolVal : def;
}

std::string JsonValue::AsString(const std::string& def) const {
    return type == JsonType::String ? strVal : def;
}

int JsonValue::AsInt(int def) const {
    return type == JsonType::Number ? static_cast<int>(numVal) : def;
}

// ==========================================
// JsonParser
// ==========================================

JsonValue JsonParser::Parse(const std::string& text) {
    JsonParser parser(text);
    JsonValue value = parser.ParseValue();
    return value;
}

JsonValue JsonParser::ParseFile(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return JsonValue();
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string text(static_cast<size_t>(len), '\0');
    fread(&text[0], 1, static_cast<size_t>(len), f);
    fclose(f);
    return Parse(text);
}

JsonParser::JsonParser(const std::string& text) : m_text(text), m_pos(0) {}

void JsonParser::SkipWhitespace() {
    while (m_pos < m_text.size()) {
        char c = m_text[m_pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            m_pos++;
        } else {
            break;
        }
    }
}

char JsonParser::Peek() {
    SkipWhitespace();
    return m_pos < m_text.size() ? m_text[m_pos] : '\0';
}

char JsonParser::Next() {
    SkipWhitespace();
    return m_pos < m_text.size() ? m_text[m_pos++] : '\0';
}

bool JsonParser::Match(char c) {
    SkipWhitespace();
    if (m_pos < m_text.size() && m_text[m_pos] == c) {
        m_pos++;
        return true;
    }
    return false;
}

JsonValue JsonParser::ParseValue() {
    char c = Peek();
    switch (c) {
        case '{': return ParseObject();
        case '[': return ParseArray();
        case '"': return ParseString();
        case 't': case 'f': return ParseKeyword();
        case 'n': return ParseKeyword();
        case '-': case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': return ParseNumber();
        default: return JsonValue();
    }
}

JsonValue JsonParser::ParseObject() {
    JsonValue obj = JsonValue::Object();
    Next(); // skip '{'
    if (Match('}')) return obj;

    do {
        JsonValue key = ParseString();
        if (key.type != JsonType::String) break;
        if (!Match(':')) break;
        JsonValue val = ParseValue();
        obj[key.strVal.c_str()] = val;
    } while (Match(','));

    if (!Match('}')) {
        // Error: expected closing brace
    }
    return obj;
}

JsonValue JsonParser::ParseArray() {
    JsonValue arr = JsonValue::Array();
    Next(); // skip '['
    if (Match(']')) return arr;

    do {
        JsonValue val = ParseValue();
        arr.arrVal.push_back(val);
    } while (Match(','));

    if (!Match(']')) {
        // Error: expected closing bracket
    }
    return arr;
}

JsonValue JsonParser::ParseString() {
    std::string result;
    Next(); // skip opening '"'
    while (m_pos < m_text.size()) {
        char c = m_text[m_pos++];
        if (c == '"') break;
        if (c == '\\') {
            if (m_pos >= m_text.size()) break;
            char esc = m_text[m_pos++];
            switch (esc) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u': {
                    // Basic unicode escape - just copy as-is for now
                    if (m_pos + 4 <= m_text.size()) {
                        result += "\\u";
                        for (int i = 0; i < 4; i++) result += m_text[m_pos++];
                    }
                    break;
                }
                default: result += esc; break;
            }
        } else {
            result += c;
        }
    }
    return JsonValue(result);
}

JsonValue JsonParser::ParseNumber() {
    size_t start = m_pos;
    if (m_pos < m_text.size() && m_text[m_pos] == '-') m_pos++;
    while (m_pos < m_text.size() && m_text[m_pos] >= '0' && m_text[m_pos] <= '9') m_pos++;
    if (m_pos < m_text.size() && m_text[m_pos] == '.') {
        m_pos++;
        while (m_pos < m_text.size() && m_text[m_pos] >= '0' && m_text[m_pos] <= '9') m_pos++;
    }
    if (m_pos < m_text.size() && (m_text[m_pos] == 'e' || m_text[m_pos] == 'E')) {
        m_pos++;
        if (m_pos < m_text.size() && (m_text[m_pos] == '+' || m_text[m_pos] == '-')) m_pos++;
        while (m_pos < m_text.size() && m_text[m_pos] >= '0' && m_text[m_pos] <= '9') m_pos++;
    }
    double val = std::atof(m_text.substr(start, m_pos - start).c_str());
    return JsonValue(val);
}

JsonValue JsonParser::ParseKeyword() {
    if (m_text.compare(m_pos, 4, "true") == 0) {
        m_pos += 4;
        return JsonValue(true);
    }
    if (m_text.compare(m_pos, 5, "false") == 0) {
        m_pos += 5;
        return JsonValue(false);
    }
    if (m_text.compare(m_pos, 4, "null") == 0) {
        m_pos += 4;
        return JsonValue(nullptr);
    }
    return JsonValue();
}

// ==========================================
// JsonWriter
// ==========================================

std::string JsonWriter::Write(const JsonValue& value, bool pretty) {
    std::string output;
    JsonWriter writer(output, pretty);
    writer.WriteValue(value, 0);
    return output;
}

JsonWriter::JsonWriter(std::string& output, bool pretty)
    : m_output(output), m_pretty(pretty) {}

void JsonWriter::WriteIndent(int indent) {
    for (int i = 0; i < indent; i++) m_output += "  ";
}

std::string JsonWriter::Escape(const std::string& str) const {
    std::string result;
    for (size_t i = 0; i < str.size(); i++) {
        char c = str[i];
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
        }
    }
    return result;
}

void JsonWriter::WriteString(const std::string& str) {
    m_output += '"';
    m_output += Escape(str);
    m_output += '"';
}

void JsonWriter::WriteValue(const JsonValue& value, int indent) {
    switch (value.type) {
        case JsonType::Null:
            m_output += "null";
            break;
        case JsonType::Bool:
            m_output += value.boolVal ? "true" : "false";
            break;
        case JsonType::Number: {
            double v = value.numVal;
            if (v == std::floor(v) && std::isfinite(v) && std::fabs(v) < 1e15) {
                m_output += std::to_string(static_cast<long long>(v));
            } else {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%.10g", v);
                m_output += buf;
            }
            break;
        }
        case JsonType::String:
            WriteString(value.strVal);
            break;
        case JsonType::Array: {
            m_output += '[';
            if (m_pretty && !value.arrVal.empty()) m_output += '\n';
            for (size_t i = 0; i < value.arrVal.size(); i++) {
                if (i > 0) {
                    m_output += ',';
                    if (m_pretty) m_output += '\n';
                }
                if (m_pretty) WriteIndent(indent + 1);
                WriteValue(value.arrVal[i], indent + 1);
            }
            if (m_pretty && !value.arrVal.empty()) { m_output += '\n'; WriteIndent(indent); }
            m_output += ']';
            break;
        }
        case JsonType::Object: {
            m_output += '{';
            if (m_pretty && !value.objVal.empty()) m_output += '\n';
            for (size_t i = 0; i < value.objVal.size(); i++) {
                if (i > 0) {
                    m_output += ',';
                    if (m_pretty) m_output += '\n';
                }
                if (m_pretty) WriteIndent(indent + 1);
                WriteString(value.objVal[i].first);
                m_output += m_pretty ? ": " : ":";
                WriteValue(value.objVal[i].second, indent + 1);
            }
            if (m_pretty && !value.objVal.empty()) { m_output += '\n'; WriteIndent(indent); }
            m_output += '}';
            break;
        }
    }
}

} // namespace Dnd
