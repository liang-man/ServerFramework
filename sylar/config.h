#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>  // 智能指针
#include <sstream> // 序列化
#include <boost/lexical_cast.hpp>   // 内存转化
#include <string>

namespace sylar {

// 定义一个基类，把一些公用的属性都放在这里面去
class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string &name, const std::string &description = "")
        : m_name(name), m_description(description) {
    }
    virtual ~ConfigVarBase(); {}

    const std::string &getName() const { return m_name; }
    const std::string &getDescription() const { return m_description; }

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string &val) = 0;   // 用参数初始化它自己的成员
protected:
    std::string m_name;
    std::string m_description;
}

// 定义具体的实现类，用一个模板类
template<class T>
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;

    ConfigVar(const std::string &name, const T &default_value, const std::string &description = "")
        : ConfigVarBase(name, description), m_val(default_value) {
    }

    std::string toString() override {
        try {
            return boost::lexical_cast<std::string>(m_val);
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception" << e.what()
                << " convert: " << typeid(m_val).name() << " to string";
        } 
        return "";
    }

    // 基础的类型转化
    bool fromString(const std::string &val) override {
        try {
            m_val = boost::lexical_cast<T>(val);
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception" << e.what()
                << " convert: string to " << typeid(m_val).name();
        }
        return false;
    } 
private:
    T m_val;
};

} // namespace syla 

#endif