#pragma once
#ifndef _MYTF
#define _MYTF

#include <cstdint>
#include <iostream>
#include <string>
#include <functional>

namespace MyTF{

template<typename T>
struct TD;

template<typename T>
  concept IsInt64 = (std::is_same_v<std::remove_pointer_t<std::remove_cvref_t<T>>, int64_t>);

template<typename T>
  concept IsString = (std::is_same_v<std::remove_pointer_t<std::remove_cvref_t<T>>, std::string>);

template<typename T>
  concept IsInt64OrString = IsInt64<T> || IsString<T>;

// 获取参数包中指定索引处的类型
template<std::size_t Index, typename... Types>
struct GetTypeAtIndex;

// 基本情况：当索引为0时，返回第一个类型
template<typename First, typename... Rest>
struct GetTypeAtIndex<0, First, Rest...> {
    using type = First;
};

// 递归情况：减少索引，并继续递归
template<std::size_t Index, typename First, typename... Rest>
struct GetTypeAtIndex<Index, First, Rest...> {
    using type = typename GetTypeAtIndex<Index - 1, Rest...>::type;
};

template<IsInt64OrString ...Args>
struct ArgsPack{

};


template <typename T>
struct function_traits;

// Specialization for lambdas and std::function
template <typename Ret, typename... Args>
struct function_traits<std::function<Ret(Args...)>> {
    using result_type = Ret;
    using argument_types = ArgsPack<Args...>;
};

template <typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)> {
    using result_type = Ret;
    using argument_types = ArgsPack<Args...>;
};


template <typename ClassType, typename Ret, typename... Args>
struct function_traits<Ret(ClassType::*)(Args...) const> {
    using result_type = Ret;
    using argument_types = ArgsPack<Args...>;
};

template <typename ClassType, typename Ret, typename... Args>
struct function_traits<Ret(ClassType::*)(Args...)> {
    using result_type = Ret;
    using argument_types = ArgsPack<Args...>;
};


// Specialization for lambda or functors
template <typename Callable>
struct function_traits : function_traits<decltype(&Callable::operator())> {};


};


#endif // !_MYTF
