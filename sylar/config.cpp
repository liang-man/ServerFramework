# include "config.h"

namespace sylar {

Config::ConfigVarMap Config::s_datas;   // 思考为什么s_datas还要早config.cpp里重新声明一下才能编译通过，否则就编译失败

}