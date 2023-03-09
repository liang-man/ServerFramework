#ifndef __SYLAR_SINGLETON_H__
#define __SYLAR_SINGLETON_H__

#include <memory>

namespace sylar {

// 之所以不在LoggerManager里实现单例模式，是因为分布式多线程很可能是每个线程一个单例
// 这是单例模式的模板类，使用模板就可以解决上述问题
template<class T, class X = void, int N = 0>
class Singleton {
public:
    static T *GetInstance() {
        static T v;
        return &v;
    }
};

template <class T, class X = void, int N = 0> 
class SingletonPtr {
public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};

}

#endif