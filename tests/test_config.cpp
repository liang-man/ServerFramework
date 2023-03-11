#include "sylar/config.h"
#include "sylar/log.h"
#include <yaml-cpp/yaml.h>

// 约定好的配置参数，在下面test_config()函数里会加载yml配置文件，解析的过程中会查找相应的约定配置，然后进行覆盖
sylar::ConfigVar<int>::ptr g_int_value_config = 
    sylar::Config::Lookup("system.port", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_float_value_config = 
    sylar::Config::Lookup("system.value", (float)10.2f, "system value");   // 跟P11最后输出结果不一样，就是这里字符串内容不一致，我自己写成了“system.port”

// 测试同名但类型不同不会报错的问题
sylar::ConfigVar<float>::ptr g_int_valuex_config = 
    sylar::Config::Lookup("system.port", (float)8080, "system port"); 

sylar::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config = 
    sylar::Config::Lookup("system.int_vec", std::vector<int>{1, 2}, "system int vec");

sylar::ConfigVar<std::list<int>>::ptr g_int_list_value_config = 
    sylar::Config::Lookup("system.int_list", std::list<int>{1, 2}, "system int list");

sylar::ConfigVar<std::set<int>>::ptr g_int_set_value_config = 
    sylar::Config::Lookup("system.int_set", std::set<int>{1, 2}, "system int set");

sylar::ConfigVar<std::unordered_set<int>>::ptr g_int_unordered_set_value_config = 
    sylar::Config::Lookup("system.int_unordered_set", std::unordered_set<int>{1, 2}, "system int unordered_set");

sylar::ConfigVar<std::map<std::string, int>>::ptr g_str_int_map_value_config = 
    sylar::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k", 2}}, "system str int map");

sylar::ConfigVar<std::unordered_map<std::string, int>>::ptr g_str_int_umap_value_config = 
    sylar::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k", 2}}, "system str int umap");

void print_yaml(const YAML::Node &node, int level)
{   
    if (node.IsScalar()) {          // 如果yml是简单类型
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type();
    } else  if (node.IsNull()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - " << level;
    } else if (node.IsMap()) {      // 如果yml是map结构
        for (auto it = node.begin(); it != node.end(); ++it) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') 
                    << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if (node.IsSequence()) {  // 如果yml是sequence
        for (size_t i = 0; i < node.size(); ++i) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml()
{
    YAML::Node root = YAML::LoadFile("/home/liangman/sylar/bin/conf/log.yml");   // 加载yaml配置源文件

    // 开始解析源文件的值
    print_yaml(root, 0);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root.Scalar();
}

void test_config()
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_float_value_config->toString();

#define XX(g_var, name, prefix) \
    { \
        auto &v = g_var->getValue(); \
        for (auto &i : v) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ": " << i; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

#define XX_M(g_var, name, prefix) \
    { \
        auto &v = g_var->getValue(); \
        for (auto &i : v) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ": {" \
                    << i.first << " - " << i.second << "}"; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_unordered_set_value_config, int_unordered_set, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);

    YAML::Node root = YAML::LoadFile("/home/liangman/sylar/bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_unordered_set_value_config, int_unordered_set, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, str_int_umap, after);
}

class Person {
public:
    Person() {}
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           << "]";
        return ss.str();
    }

    bool operator==(const Person &oth) const {
        return m_name == oth.m_name
            && m_age == oth.m_age
            && m_sex == oth.m_sex;
    }
};

// 给Person加偏特化
namespace sylar {
template<>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();

        return p;
    }
};

template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person &p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

}

// 如果只是这样写的化，编译会不通过，因为是模板，不知道怎么去转换，解决办法就是加一个偏特化(编译错误信息见"Person未加偏特化报错信息.png")
// 测试自定义结构类型
sylar::ConfigVar<Person>::ptr g_person = 
    sylar::Config::Lookup("class.person", Person(), "system person");

// 测试自定义结构嵌套进map
sylar::ConfigVar<std::map<std::string, Person>>::ptr g_person_map = 
    sylar::Config::Lookup("class.map", std::map<std::string, Person>(), "system map");

sylar::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_person_vec_map = 
    sylar::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person>>(), "system vec map");

void test_class()
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix) \
    { \
        auto m = g_person_map->getValue(); \
        for (auto &i : m) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << prefix << ": " << i.first << " - " << i.second.toString(); \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << prefix << ": size=" << m.size(); \
    }

    g_person->addListener(10, [](const Person &old_value, const Person &new_value){
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "old_value=" << old_value.toString()
                << " new value=" << new_value.toString();
    });

    XX_PM(g_person_map, "class.map before");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("/home/liangman/sylar/bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();
    XX_PM(g_person_map, "class.map after");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_person_vec_map->toString();
}

int main(int argc, char **argv)
{
    // test_yaml();
    // test_config();
    test_class();

    return 0;
}