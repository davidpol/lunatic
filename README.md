# lunatic

lunatic is a collection of utilities for mixed C++ and Lua development.

### lua_function

_lua_function_ is a class template that allows calling Lua functions from C++ in a very simple way. It generates the appropriate Lua stack-handling code automatically at compile-time. Consider you have a valid _lua_State*_ containing the following definition:

    function sum(a, b)
        return a + b
    end

Calling this Lua function from C++ is as easy as writing this code:

	lunatic::lua_function<int> sum(L, "sum");
	const int result = sum(3, 7);

Where _L_ is a pointer to an initialized _lua_State_. Note that _lua_function_ is defined in the _lua_function.hpp_ header file and lives inside the _lunatic_ namespace. The parameter types are automatically deduced and the template arguments indicate the types of the return values. You can use void for functions that return no value.

_lua_function_ is a variadic template, so it also supports calling Lua functions that return multiple values. These values are conveniently packed in a _std::tuple_. Consider this Lua function:

	function some_primes()
		return 2, 3, 5
	end

We can call it from C++ like this:

	lunatic::lua_function<int, int, int> some_primes(L, "some_primes");
    const std::tuple<int, int, int> result = some_primes();
