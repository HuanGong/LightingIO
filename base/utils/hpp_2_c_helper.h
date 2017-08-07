#ifndef _CPP_CALLBACK_2_C_CALLBACK_H_
#define _CPP_CALLBACK_2_C_CALLBACK_H_


#include <functional>

template <typename T>
struct Callback;

template <typename Ret, typename... Params>
struct Callback<Ret(Params...)> {
  template <typename... Args>
  static Ret callback(Args... args) { func(args...); }
  static std::function<Ret(Params...)> func;
};

template <typename Ret, typename... Params>
std::function<Ret(Params...)> Callback<Ret(Params...)>::func;

#endif
