# include "config.h"

namespace sylar {

// 思考为什么s_datas还要早config.cpp里重新声明一下才能编译通过，否则就编译失败
// 答：这里是对类的静态成员变量的初始化
/*
 * 知识点：
 * 1.静态数据成员不能在类中初始化，一般在类外和main()函数之前初始化，缺省时初始化为0。
 * 2.static成员的所有者是类本身，但是多个对象拥有一样的静态成员。从而在定义对象是不能通过构造函数对其进行初始化。
 * 3.静态成员不能在类定义里边初始化，只能在class body外初始化。
 * 4.静态成员仍然遵循public，private，protected访问准则。
 * 5.静态成员函数没有this指针，它不能访问非静态成员，因为除了对象会调用它外，类本身也可以调用(非静态成员函数可以访问静态成员)
 * 6.静态成员属于全局变量，是所有实例化以后的对象所共享的，而成员的初始化你可以想象成向系统申请内存存储数据的过程，显然这种共有对象，不能在任何函数和局部作用域中初始化。
 * 7.用于修饰类的数据成员，即所谓“静态成员”。这种数据成员的生存期大于class的对象（实例/instance）。静态数据成员是每个class有一份，普通数据成员是每个instance 有一份。
*/
// Config::ConfigVarMap Config::s_datas;    // 采用静态成员函数初始化s_datas后，就不用在这里再初始化了

ConfigVarBase::ptr Config::LookupBase(const std::string &name)
{
    MutexType::ReadLock lock(GetMutex());
    // auto it = s_datas.find(name);
    // return it == s_datas.end() ? nullptr : it->second;
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

// "A.B", 10
// A:
//   B: 10
//   C: str
// 解析成树会不会更好？
static void ListAllMember(const std::string &prefix, 
                          const YAML::Node &node,
                          std::list<std::pair<std::string, const YAML::Node>> &output)
{
    // 如果是非法字符
    if (prefix.find_first_not_of("abcdefggijklmnopqrstuvwxyz._012345678") != std::string::npos) {
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invaild name: " << prefix << " : " << node;
        return;   
    }
    // 如果不是非法字符
    output.push_back(std::make_pair(prefix, node));
    if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}

void Config::LoadFromYaml(const YAML::Node &root)
{
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    ListAllMember("", root, all_nodes);

    for (auto &i : all_nodes) {
        std::string key = i.first;
        if (key.empty()) {
            continue;
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);

        if (var) {
            if (i.second.IsScalar()) {
                var->fromString(i.second.Scalar());
            } else {
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb)
{
    MutexType::ReadLock lock(GetMutex());
    ConfigVarMap &m = GetDatas();
    for (auto it = m.begin(); it != m.end(); ++it) {
        cb(it->second);
    }
}

}