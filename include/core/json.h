#pragma once

#include "core/types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdio>
#include <cstdlib>
#include <cmath>

namespace Dnd {

enum class JsonType : u8 {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

class JsonValue {
public:
    JsonType type;
    bool boolVal;
    double numVal;
    std::string strVal;
    std::vector<JsonValue> arrVal;
    std::vector<std::pair<std::string, JsonValue>> objVal;

    JsonValue();
    JsonValue(std::nullptr_t);
    JsonValue(bool val);
    JsonValue(double val);
    JsonValue(int val);
    JsonValue(const char* val);
    JsonValue(const std::string& val);

    static JsonValue Array();
    static JsonValue Object();

    bool IsNull() const;

    JsonValue& operator[](const char* key);
    const JsonValue& operator[](const char* key) const;
    JsonValue& operator[](size_t idx);
    const JsonValue& operator[](size_t idx) const;
    size_t Size() const;

    bool Has(const char* key) const;
    JsonValue* Get(const char* key);
    const JsonValue* Get(const char* key) const;

    double GetNumber(const char* key, double def = 0.0) const;
    bool GetBool(const char* key, bool def = false) const;
    std::string GetString(const char* key, const std::string& def = "") const;
    int GetInt(const char* key, int def = 0) const;

    double AsNumber(double def = 0.0) const;
    bool AsBool(bool def = false) const;
    std::string AsString(const std::string& def = "") const;
    int AsInt(int def = 0) const;
};

class JsonParser {
public:
    static JsonValue Parse(const std::string& text);
    static JsonValue ParseFile(const char* path);
private:
    JsonParser(const std::string& text);
    JsonValue ParseValue();
    JsonValue ParseObject();
    JsonValue ParseArray();
    JsonValue ParseString();
    JsonValue ParseNumber();
    JsonValue ParseKeyword();
    void SkipWhitespace();
    char Peek();
    char Next();
    bool Match(char c);

    const std::string& m_text;
    size_t m_pos;
};

class JsonWriter {
public:
    static std::string Write(const JsonValue& value, bool pretty = true);
private:
    JsonWriter(std::string& output, bool pretty);
    void WriteValue(const JsonValue& value, int indent);
    void WriteString(const std::string& str);
    void WriteIndent(int indent);
    std::string Escape(const std::string& str) const;

    std::string& m_output;
    bool m_pretty;
};

} // namespace Dnd
