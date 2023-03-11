#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>  // 智能指针
#include <sstream> // 序列化
#include <boost/lexical_cast.hpp>   // 内存转化  boost库的安装见收藏夹，库默认安装在/usr/local/include中，这样就可以使用#include <boost/...>了
#include <string>
#include <map>
#include <vector>
#include <list>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <exception>
#include "log.h"
#include <yaml-cpp/yaml.h>
#include <functional>

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
    virtual std::string getTypeName() const = 0;
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

// list的偏特化
template<class T>
class LexicalCast<std::string, std::list<T>> {
public:
    std::list<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
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
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T> &v) {
        YAML::Node node;
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// set的偏特化
template<class T>
class LexicalCast<std::string, std::set<T>> {
public:
    std::set<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T> &v) {
        YAML::Node node;
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// unordered_set的偏特化
template<class T>
class LexicalCast<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T> &v) {
        YAML::Node node;
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// map的偏特化
template<class T>
class LexicalCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T> &v) {
        YAML::Node node;
        for (auto &i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// unordered_map的偏特化
template<class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T> &v) {
        YAML::Node node;
        for (auto &i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
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
    /*
     * c++11里function很好用，可以封装成指针、lambda格式、以及成员或者伪装，把一个函数签名不一样的给包装成一个满足需要的接口
     * 与智能指针一样，蛮有必要去了解一下
     * 当一个配置更改的时候，我们应该通知到，让他知道原来的值是什么，新值是什么，然后基于原来的值做一些清理，根据新的值做一些修改
    */
    typedef std::function<void (const T &old_value, const T &new_value)> on_change_callback;

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
    // void setValue(const T &val) { m_val = val; }
    // 我们要通知变化，比如100项配置里，我们只改了1项，那么没变化的没必要让用户知道，所以要知道原值和新值是否发生了变化
    void setValue(const T &val) { 
        if (val == m_val) {
            return;
        }
        for (auto &i : m_cbs) {
            i.second(m_val, val);
        }
        m_val = val;
    }
    std::string getTypeName() const override { return typeid(T).name(); }

    // 监听
    void addListener(uint64_t key, on_change_callback cb) {
        m_cbs[key] = cb;
    }

    // 删除
    void delListener(uint64_t key) {
        m_cbs.erase(key);
    }

    on_change_callback getListener(uint64_t key) { 
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second; 
    } 

    // 清空
    void clearListener() {
        m_cbs.clear();
    }
private:
    T m_val; 

    // 变更回调函数组
    // 为什么要用map？因为function没有比较函数，如果我们想在vector里判断是否是同一个function，是没法判断的，因为没有比较函数
    // 所以要用map加一个关键字标签key(uint64_t)，要求唯一，一般可以用hash值
    // 为此还需要增加监听
    std::map<uint64_t, on_change_callback> m_cbs;
};

// 实现创建和查找日志
class Config {
public:
    typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name, const T &default_vale,
            const std::string &description = "") {
        ////////////////对下面注释的改变//////////////
        // 这样做了之后，有问题就会爆出这个问题
        auto it = s_datas.find(name);
        if (it != s_datas.end()) {
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);  // 利用dynamic_pointer_cast转换类型时，如果同名key但值类型不同，转换会失败，返回nullptr
            if (tmp) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists";
                return tmp;
            } else {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists but type not "
                        << typeid(T).name() << " real_type=" << it->second->getTypeName()
                        << " " << it->second->toString();
                return nullptr;
            }
        }
        // auto tmp = Lookup<T>(name);
        // if (tmp) {
        //     SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists";
        //     return tmp;
        // }

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