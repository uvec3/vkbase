#pragma once
//replaces member function name with lambda which calls to the function with captured "this"
#define WRAP_MEMBER_FUNC(func_name) [this](auto&&... args) {return func_name(args...);}