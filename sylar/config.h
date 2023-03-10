#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>  // 智能指针
#include <sstream> // 序列化
#include <boost/lexical_cast.hpp>   // 内存转化  boost库的安装见收藏夹，库默认安装在/usr/local/include中，这样就可以使用#include <boost/...>了
#include <string>
#include <map>
#include <exception>
#include "log.h"
#include <yaml-cpp/yaml.h>

namespace sylar {

// 定义一个基类，把一些公用的属性都放在这里面去
class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    // 定义的时候直接转成小写，这样就只有小写没有大写
    ConfigVarBase(const std::string &name, const std::string &description = "")
            : m_name(name)
            , m_description(description) {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }
    virtual ~ConfigVarBase() {}

    const std::string &getName() const { return m_name; }
    const std::string &getDescription() const { return m_description; }

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string &val) = 0;   // 用参数初始化它自己的成员
protected:
    std::string m_name;
    std::string m_description;
};

// F -- from_type  F类型去转换成string类型
// T -- to_type    string类型去转换成T类型
// 通用的简单的类型转换
template<class F, class T>
class LexicalCast {
public:
    T operator()(const F &v) {
        return boost::lexical_cast<T>(v);
    }
};

// vector的偏特化
template<class T>
class LexicalCast<std::string, std::vector<T>> {
public:
    std::vector<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T> &v) {
        YAML::Node node;
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// 定义具体的实现类，用一个模板类
// 支持序列化和反序列化，一种是转成string，另一种是把string转为我们要的类型
// FromStr T operator()(const std::string &)
// ToStr std::string operator()(const T &) 
template<class T, class FromStr = LexicalCast<std::string, T>   // 类的特例化
                , class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;

    ConfigVar(const std::string &name, const T &default_value, const std::string &description = "")
        : ConfigVarBase(name, description), m_val(default_value) {
    }

    // 外部的类型转成string类型进行解析
    std::string toString() override {
        try {
            // return boost::lexical_cast<std::string>(m_val);   // 简单版本，只支持简单类型  而对于基础类型的转换，由新定义的通用的基础类型转换类来做，这样不至于说加了复杂类型转换，简单类型转换就不支持了
            return ToStr()(m_val);        // 复杂版本
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception" << e.what()
                << " convert: " << typeid(m_val).name() << " to string";
        } 
        return "";
    }

    // 基础的类型转化
    bool fromString(const std::string &val) override {
        try {
            // m_val = boost::lexical_cast<T>(val);   // 简单版本，只支持简单类型
            setValue(FromStr()(val));        // 复杂版本
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception" << e.what()
                << " convert: string to " << typeid(m_val).name();
        }
        return false;
    } 

    const T getValue() const { return m_val; }
    void setValue(const T &val) { m_val = val; }
private:
    T m_val;
};

// 实现创建和查找日志
class Config {
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name, const T &default_vale,
            const std::string &description = "") {
        auto tmp = Lookup<T>(name);
        if (tmp) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists";
            return tmp;
        }

        if (name.find_first_not_of("abcdefggijklmnopqrstuvwxyz._012345678")
                != std::string::npos) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }

        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_vale, description));
        s_datas[name] = v;

        return v;
    }

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
        auto it = s_datas.find(name);
        if (it == s_datas.end()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    static void LoadFromYaml(const YAML::Node &root);
    static ConfigVarBase::ptr LookupBase(const std::string &name);
private:
    static ConfigVarMap s_datas;
};

} // namespace syla 

#endif