/**
 * @file hook.h
 * @brief hook函数封装
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-06-02
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */

#ifndef __SYLAR_HOOK_H__
#define __SYLAR_HOOK_H__

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

namespace sylar {
    /**
     * @brief 当前线程是否hook   hook的操作比较细，只要某个线程被hook了，那么这个线程下的所有操作都支持hook，都会替换成我们自己的实现
     */
    bool is_hook_enable();
    /**
     * @brief 设置当前线程的hook状态
     */
    void set_hook_enable(bool flag);
}

extern "C" {    // 表示这里面的函数是用c风格来写，这样编译就不会被编译成C++风格，因为编译成C++风格会多一些乱七八糟的东西(查：编译C代码与C++代码的区别？)

//sleep
typedef unsigned int (*sleep_fun)(unsigned int seconds);   // 定义一个函数签名
extern sleep_fun sleep_f;    // 定义一个sleep_fun类型的函数指针sleep_f，sleep_f就是原来系统的实现

typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

}

#endif