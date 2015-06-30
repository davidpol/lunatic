#ifndef LUNATIC_LUA_FUNCTION_HPP_INCLUDED
#define LUNATIC_LUA_FUNCTION_HPP_INCLUDED

#include "lua.hpp"

#include <cassert>
#include <string>
#include <tuple>
#include <utility>

namespace lunatic
{
    // Helper templates to push values from C++ to the Lua stack.
    // You can define your own specializations for other types.
    template<typename T>
    void push_argument(lua_State* L, T arg);

    template<typename T, typename... Args>
    void push_argument(lua_State* L, T first, Args&&... args)
    {
        push_argument(L, first);
        push_argument(L, std::forward<Args>(args)...);
    }

    template<>
    void push_argument<bool>(lua_State* L, bool arg)
    {
        lua_pushboolean(L, arg ? 1 : 0);
    }

    template<>
    void push_argument<int>(lua_State* L, int arg)
    {
        lua_pushinteger(L, arg);
    }

    template<>
    void push_argument<float>(lua_State* L, float arg)
    {
        lua_pushnumber(L, arg);
    }

    template<>
    void push_argument<double>(lua_State* L, double arg)
    {
        lua_pushnumber(L, arg);
    }

    template<>
    void push_argument<std::string>(lua_State* L, std::string arg)
    {
        lua_pushstring(L, arg.c_str());
    }

    // Helper templates to retrieve values from Lua into C++-land.
    // You can define your own specializations for other types.
    template<typename Ret>
    Ret pop_argument(lua_State* L, int index);

    template<>
    bool pop_argument<bool>(lua_State* L, int index)
    {
        return lua_toboolean(L, index);
    }

    template<>
    int pop_argument<int>(lua_State* L, int index)
    {
        return int(lua_tointeger(L, index));
    }

    template<>
    float pop_argument<float>(lua_State* L, int index)
    {
        return float(lua_tonumber(L, index));
    }

    template<>
    double pop_argument<double>(lua_State* L, int index)
    {
        return double(lua_tonumber(L, index));
    }

    template<>
    std::string pop_argument<std::string>(lua_State* L, int index)
    {
        return lua_tostring(L, index);
    }

    template<typename Ret>
    std::tuple<Ret> pop_arguments(lua_State* L, int index)
    {
        return std::make_tuple(pop_argument<Ret>(L, index));
    }

    template<typename Ret1, typename Ret2, typename... RetN>
    std::tuple<Ret1, Ret2, RetN...> pop_arguments(lua_State* L, int index)
    {
        const auto head = std::make_tuple(pop_argument<Ret1>(L, index));
        return std::tuple_cat(head, pop_arguments<Ret2, RetN...>(L, index + 1));
    }

    // Convenience base class for lua_function.
    namespace detail
    {
        class lua_function_base
        {
        public:
            lua_function_base(lua_State* L, const std::string& name) :
            state(L), name(name) {}

            lua_function_base(const lua_function_base&) = delete;
            lua_function_base& operator=(const lua_function_base&) = delete;

            lua_State* state;
            std::string name;

        protected:
            void get_global_function()
            {
                lua_getglobal(state, name.c_str());
                assert(lua_isfunction(state, -1));
            }
        };
    }

    // This class represents a Lua function that is callable from C++.
    template<typename... Ret>
    class lua_function;

    template<typename Ret>
    class lua_function<Ret> : public detail::lua_function_base
    {
    public:
        lua_function(lua_State* L, const std::string& name) :
        lua_function_base(L, name) {}

        Ret operator()()
        {
            get_global_function();
            return call_lua_func(0);
        }

        template<typename T>
        Ret operator()(T arg)
        {
            get_global_function();
            push_argument(state, arg);
            return call_lua_func(1);
        }

        template<typename T, typename... Args>
        Ret operator()(T first, Args&&... args)
        {
            get_global_function();
            push_argument(state, first);
            push_argument(state, std::forward<Args>(args)...);
            return call_lua_func(sizeof...(Args) + 1);
        }

    private:
        Ret call_lua_func(int num_args)
        {
            const auto call_ret = lua_pcall(state, num_args, 1, 0);
            assert(call_ret == 0);
            const auto ret = pop_argument<Ret>(state, -1);
            lua_pop(state, 1);
            return ret;
        }
    };

    // This class template specialization implements support for functions
    // that return multiple values.
    template<typename Ret1, typename... RetN>
    class lua_function<Ret1, RetN...> : public detail::lua_function_base
    {
    public:
        lua_function(lua_State* L, const std::string& name) :
        lua_function_base(L, name) {}

        std::tuple<Ret1, RetN...> operator()()
        {
            get_global_function();
            return call_lua_func(0);
        }

        template<typename T>
        std::tuple<Ret1, RetN...> operator()(T arg)
        {
            get_global_function();
            push_argument(state, arg);
            return call_lua_func(1);
        }

        template<typename T, typename... Args>
        std::tuple<Ret1, RetN...> operator()(T first, Args&&... args)
        {
            get_global_function();
            push_argument(state, first);
            push_argument(state, std::forward<Args>(args)...);
            return call_lua_func(sizeof...(Args) + 1);
        }

    private:
        std::tuple<Ret1, RetN...> call_lua_func(int num_args)
        {
            const int num_ret_values = sizeof...(RetN) + 1;
            const auto call_ret = lua_pcall(state, num_args, num_ret_values, 0);
            assert(call_ret == 0);
            const auto ret = pop_arguments<Ret1, RetN...>(state, -num_ret_values);
            lua_pop(state, num_ret_values);
            return ret;
        }
    };

    // This class template specialization implements support for functions
    // that return no value.
    template<>
    class lua_function<void> : public detail::lua_function_base
    {
    public:
        lua_function(lua_State* L, const std::string& name) :
        lua_function_base(L, name) {}

        void operator()()
        {
            get_global_function();
            call_lua_func(0);
        }

        template<typename T>
        void operator()(T arg)
        {
            get_global_function();
            push_argument(state, arg);
            call_lua_func(1);
        }

        template<typename T, typename... Args>
        void operator()(T first, Args&&... args)
        {
            get_global_function();
            push_argument(state, first);
            push_argument(state, std::forward<Args>(args)...);
            call_lua_func(sizeof...(Args) + 1);
        }

    private:
        void call_lua_func(int num_args)
        {
            const auto call_ret = lua_pcall(state, num_args, 0, 0);
            assert(call_ret == 0);
        }
    };

}

#endif
