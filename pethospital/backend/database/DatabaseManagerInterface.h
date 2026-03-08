// 数据库管理接口规范和协议定义

/*
#ifndef IBASE_DATABASE_H
检查是否未定义 IBASE_DATABASE_H 宏。

#define IBASE_DATABASE_H
.....(没有被包含时执行的代码)
#endif
如果已经定义过（即该头文件之前已被包含过），则跳过到#endif之后的代码部分。
=> #pragma once
*/
#ifndef IBASE_DATABASE_H
#define IBASE_DATABASE_H

#include "mysqlx/xdevapi.h"

class DatabaseManagerInterface
{
public:
    virtual ~DatabaseManagerInterface() = default;

    // C++中的虚函数机制允许通过基类指针或引用来调用派生类中重写的函数
    
    virtual mysqlx::Session *getSession() = 0; // 纯虚函数
    virtual mysqlx::Schema *getSchema() = 0;   // 纯虚函数
    virtual void create_Tables() = 0;          // 纯虚函数
};

#endif