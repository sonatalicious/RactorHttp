#pragma once
#include <functional>

// 定义函数指针
//typedef	int(*handleFunc)(void* arg);
//using handleFunc = int(*)(void* ); // C++ 11 新特性 using

// 定义文件描述符的读写事件
// 改为强类型枚举	
enum class FDEvent
{
	TimeOut = 0x01,
	ReadEvent = 0x02,
	WriteEvent = 0x04
};

// 可调用对象包装器打包的是什？ 1、函数指针 2、可调用对象（可以像函数一样使用的对象）
// 最终得到了地址，但是没有调用，下面的例子中 "int(void*)" 为函数签名
// std::function 允许你打包并存储任意与此签名匹配的可调用对象
// 可调用对象包装器是一个类模板，它的名字叫 function
// 下面将使用 function 对 int(*)(void* )进行封装
// std::function 和 std::bind 常常在项目中组合使用，写有关回调函数代码时，常常使用
// 要注意,eventLoop 中就用到了
// std::bind 是一个函数模板，将可调用对象和它的参数绑定在一起，并用 std::function 包装并返回

class Channel
{
public:
	// 用可调用对象包装器，包装一下 int(void*) ，然后给这个包装器类起一个 handleFunc 别名
	using handleFunc = std::function<int(void*)>; 
	Channel(int fd, FDEvent events, handleFunc readFunc,\
		handleFunc writeFunc, handleFunc destroyFunc, void* arg);
	// 回调函数
	handleFunc readCallback;
	handleFunc writeCallback;
	handleFunc destroyCallback;
	// 修改 fd 写事件（检测 or 不检测）
	void writeEventEnable( bool flag);
	// 判断是否需要检测文件描述符的写事件
	bool isWriteEventEnable();
	// 取出私有成员的值
	inline int getEvent() // inline 不压栈，直接替换代码块
	{
		return this->m_events;
	}
	inline int getSocket()
	{
		return this->m_fd;
	}
	inline const void* getArg()
	{
		return this->m_arg;
	}
private:
	// 文件描述符
	int m_fd;
	// 事件
	int m_events;
	// 回调函数的参数
	void* m_arg;
};


