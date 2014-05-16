#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <assert.h>

using namespace std;

struct EnumType {
    struct Item {string key, name, value, desc, data;};
    EnumType() {continuous = true;}
    string key, name, desc, data;
    bool continuous;
    vector<Item> items;
    int default_ = 0;
};

static inline std::string trim(const std::string &str) {
    const string spaces("     \f\v\n\r");
    const int from = str.find_first_not_of(spaces);
    const int to = str.find_last_not_of(spaces);
    return (from <= to && from != string::npos) ? str.substr(from, to+1-from) : string();
}

static string getTemplate(const string &fileName) {
    fstream in;
    in.open(fileName.c_str(), ios::in);
    assert(in.is_open());
    string line;
    stringstream out;
    while (std::getline(in, line)) {
        cout << trim(line) << endl;
        if (!trim(line).empty() && line[0] != '#') {
            out << line << endl;
        }
    }
    return out.str();
}

static inline int toInt(const string &s) {
    stringstream str;
    str.str(s);
    int number;
    str >> number;
    return number;
}

static inline string toString(int n) {
    stringstream str;
    str << n;
    return str.str();
}

static string addOne(const string &value) {
    int idx = value.size()-1;
    for (; idx >= 0; --idx) {
        const char c = value[idx];
        if (!('0' <= c && c <= '9'))
            break;
    }
    stringstream str;
    str << (toInt(idx < 0 ? value : value.substr(idx+1))+1);
    return value.substr(0, idx+1) + str.str();
}

static vector<EnumType> readEnums() {
    fstream in;
    in.open("enum-list", ios::in);
    assert(in.is_open());

    auto substr = [] (std::string &str, const std::string &open, const string &close) {
        auto from = str.find(open);
    if (from == string::npos)
        return string();
    from += open.size();
        auto to   = str.find(close, from);
    if (to == string::npos)
        return string();
        return str.substr(from, to - from);
    };

    vector<EnumType> type;
    string line;
    while (getline(in, line)) {
        line = trim(line);
        if (line.empty())
            continue;
        bool isType = false;
        if (line[0] == '+')
            isType = true;
        else if (line[0] == '-')
            isType = false;
        else
            continue;
        line = line.substr(1);
        const auto def = line.front() == '*';
        if (def)
            line = line.substr(1);
        const auto key = substr(line, "[[", "]]");
        const auto value = substr(line, "[=", "=]");
        const auto desc = substr(line, "[-", "-]");
        const auto data = substr(line, "[:", ":]");
        const auto idx = {line.find("[["), line.find("[="), line.find("[-"), line.find("[:")};
        const auto name = line.substr(0, *std::min_element(std::begin(idx), std::end(idx)));
        if (isType) {
            type.push_back(EnumType());
            auto &one = type.back();
            one.name = name;
            one.key = key;
            one.desc = desc;
            one.data = data.empty() ? "QVariant" : data;
        } else {
            auto &items = type.back().items;
            items.push_back(EnumType::Item());
            auto &item = items.back();
            item.name = name;
            item.key = key;
            item.desc = desc;
            if (!value.empty()) {
                item.value = value;
                type.back().continuous = false;
            } else {
                const size_t size = items.size();
                item.value = size == 1u ? string("0") : addOne(items[size-2].value);
            }
            if (def)
                type.back().default_ = items.size()-1;
            item.data = data.empty() ? ("(int)" + item.value) : data;
        }
    }
    return type;
}

static string &replace(string &str, const string &substr, const string &overwrite) {
    string::size_type from = 0;
    for (;;) {
        from = str.find(substr, from);
        if (from == str.npos)
            break;
        str.replace(from, substr.size(), overwrite);
        from += overwrite.size();
    }
    return str;
}

static const string hppTmp = R"(
enum class __ENUM_NAME : int {
__ENUM_VALUES
};

inline auto operator == (__ENUM_NAME e, int i) -> bool { return (int)e == i; }
inline auto operator != (__ENUM_NAME e, int i) -> bool { return (int)e != i; }
inline auto operator == (int i, __ENUM_NAME e) -> bool { return (int)e == i; }
inline auto operator != (int i, __ENUM_NAME e) -> bool { return (int)e != i; }
inline auto operator & (__ENUM_NAME e, int i) -> int { return (int)e & i; }
inline auto operator & (int i, __ENUM_NAME e) -> int { return (int)e & i; }
inline auto operator &= (int &i, __ENUM_NAME e) -> int& { return i &= (int)e; }
inline auto operator ~ (__ENUM_NAME e) -> int { return ~(int)e; }
inline auto operator | (__ENUM_NAME e, int i) -> int { return (int)e | i; }
inline auto operator | (int i, __ENUM_NAME e) -> int { return (int)e | i; }
constexpr inline auto operator | (__ENUM_NAME e1, __ENUM_NAME e2) -> int { return (int)e1 | (int)e2; }
inline auto operator |= (int &i, __ENUM_NAME e) -> int& { return i |= (int)e; }
inline auto operator > (__ENUM_NAME e, int i) -> bool { return (int)e > i; }
inline auto operator < (__ENUM_NAME e, int i) -> bool { return (int)e < i; }
inline auto operator >= (__ENUM_NAME e, int i) -> bool { return (int)e >= i; }
inline auto operator <= (__ENUM_NAME e, int i) -> bool { return (int)e <= i; }
inline auto operator > (int i, __ENUM_NAME e) -> bool { return i > (int)e; }
inline auto operator < (int i, __ENUM_NAME e) -> bool { return i < (int)e; }
inline auto operator >= (int i, __ENUM_NAME e) -> bool { return i >= (int)e; }
inline auto operator <= (int i, __ENUM_NAME e) -> bool { return i <= (int)e; }

Q_DECLARE_METATYPE(__ENUM_NAME)

template<>
class EnumInfo<__ENUM_NAME> {
    typedef __ENUM_NAME Enum;
public:
    typedef __ENUM_NAME type;
    using Data =  __ENUM_DATA_TYPE;
    struct Item {
        Enum value;
        QString name, key;
        __ENUM_DATA_TYPE data;
    };
    using ItemList = std::array<Item, __ENUM_COUNT>;
    static constexpr auto size() -> int
    { return __ENUM_COUNT; }
    static constexpr auto typeName() -> const char*
    { return "__ENUM_NAME"; }
    static constexpr auto typeKey() -> const char*
    { return "__ENUM_KEY"; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", "__ENUM_DESC"); }
    static auto item(Enum e) -> const Item*
    { __ENUM_FUNC_ITEM }
    static auto name(Enum e) -> QString
    { auto i = item(e); return i ? i->name : QString(); }
    static auto key(Enum e) -> QString
    { auto i = item(e); return i ? i->key : QString(); }
    static auto data(Enum e) -> __ENUM_DATA_TYPE
    { auto i = item(e); return i ? i->data : __ENUM_DATA_TYPE(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
__ENUM_FUNC_DESC_CASES
        }
    }
    static constexpr auto items() -> const ItemList&
    { return info; }
    static auto from(int id, Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [id] (const Item &item)
                               { return item.value == id; });
        return it != info.cend() ? it->value : def;
    }
    static auto from(const QString &name, Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&name] (const Item &item)
                               { return !name.compare(item.name); });
        return it != info.cend() ? it->value : def;
    }
    static auto fromData(const __ENUM_DATA_TYPE &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return __ENUM_NAME::__ENUM_DEFAULT; }
private:
    static const ItemList info;
};

using __ENUM_NAMEInfo = EnumInfo<__ENUM_NAME>;
)";

static const string cppTmp = R"(
const std::array<__ENUM_NAMEInfo::Item, __ENUM_COUNT> __ENUM_NAMEInfo::info{{
__ENUM_INFOS
}};
)";

static void generate() {
    cout << "Generate enum files" << endl;
    const auto enums = readEnums();
    string hpp = R"(
#ifndef ENUMS_HPP
#define ENUMS_HPP

#include <QCoreApplication>
#include <array>
#include "video/videocolor.hpp"
extern "C" {
#include <audio/chmap.h>
}

template<typename T> class EnumInfo { static constexpr int size() { return 0; } double dummy; };

typedef QString (*EnumVariantToSqlFunc)(const QVariant &var);
typedef QVariant (*EnumVariantFromSqlFunc)(const QVariant &var, const QVariant &def);

template<typename T>
QString _EnumVariantToSql(const QVariant &var) {
    Q_ASSERT(var.userType() == qMetaTypeId<T>());
    return QLatin1Char('\'') % EnumInfo<T>::name(var.value<T>()) % QLatin1Char('\'');
}

template<typename T>
QVariant _EnumVariantFromSql(const QVariant &name, const QVariant &def) {
    const auto enum_ = EnumInfo<T>::from(name.toString(), def.value<T>());
    return QVariant::fromValue<T>(enum_);
}

template<class Enum>
using EnumData = typename EnumInfo<Enum>::Data;

template<class Enum>
static inline auto _GetEnumData(Enum e) -> EnumData<Enum>
    { return EnumInfo<Enum>::data(e); }

)";

    const string convtmpl = "    if (varType == qMetaTypeId<__ENUM_NAME>()) {\n        toSql = _EnumVariantToSql<__ENUM_NAME>;\n        fromSql = _EnumVariantFromSql<__ENUM_NAME>;\n    } else";
    string enumConv = "static inline bool _GetEnumFunctionsForSql(int varType, EnumVariantToSqlFunc &toSql, EnumVariantFromSqlFunc &fromSql) {\n";
    string isEnum = "static inline bool _IsEnumTypeId(int userType) {\n    return ";
    string cpp = "#include \"enums.hpp\"";
    for (const EnumType &type : enums) {
        string htmpl = hppTmp;
        string ctmpl = cppTmp;
        replace(htmpl, "__ENUM_NAME", type.name);
        replace(htmpl, "__ENUM_KEY", type.key);
        replace(htmpl, "__ENUM_DESC", type.desc);
        replace(htmpl, "__ENUM_DEFAULT", type.items[type.default_].name);
        replace(htmpl, "__ENUM_DATA_TYPE", type.data);
        string value, infos, cases;
        for (const EnumType::Item &item : type.items) {
            value += "    " + item.name + " = (int)" + item.value;
            infos += "    {" + type.name + "::" + item.name + ", " + '"' + item.name + "\", \"" + item.key + "\", " + item.data + "}";
            cases += "        case Enum::" + item.name + ": return qApp->translate(\"EnumInfo\", \"" + item.desc + "\");\n";
            if (&item != &type.items.back()) {
                value += ",\n";
                infos += ",\n";
            }
        }
        cases += "        default: return \"\";";
        replace(htmpl, "__ENUM_VALUES", value);
        replace(htmpl, "__ENUM_COUNT", toString(type.items.size()));
        replace(htmpl, "__ENUM_FUNC_DESC_CASES", cases);
        if (type.continuous)
            replace(htmpl, "__ENUM_FUNC_ITEM", R"(return 0 <= e && e < size() ? &info[(int)e] : nullptr;)");
        else
            replace(htmpl, "__ENUM_FUNC_ITEM", R"(
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [e] (const Item &info)
                               { return info.value == e; });
        return it != info.cend() ? &(*it) : nullptr;
   )");

        replace(ctmpl, "__ENUM_NAME", type.name);
        replace(ctmpl, "__ENUM_COUNT", toString(type.items.size()));
        replace(ctmpl, "__ENUM_INFOS", infos);

        hpp += htmpl;
        cpp += ctmpl;

        string conv = convtmpl;
        enumConv += replace(conv, "__ENUM_NAME", type.name);
        isEnum += "userType == qMetaTypeId<" + type.name + ">()\n        || ";
    }
    isEnum += "false;\n}\n\n";
    enumConv += "\n        return false;\n    return true;\n}";
    hpp += isEnum;
    hpp += enumConv;
    hpp += "\n#endif\n";
    fstream s_hpp, s_cpp;
    s_hpp.open("../enums.hpp", ios::out);
    s_cpp.open("../enums.cpp", ios::out);
    s_hpp << hpp;
    s_cpp << cpp;

    cout << hpp;
}

int main() {
    generate();
    return 0;
}
