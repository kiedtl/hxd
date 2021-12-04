#include <assert.h>
#include <err.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#if LUA_VERSION_NUM < 502
#error "huxdemp requires Lua >= v5.2."
#endif

#define luau_rawlen(ST, NM) lua_rawlen(ST, NM)
#define luau_setfuncs(ST, FN) luaL_setfuncs(ST,   FN,  0);

static lua_State *L = NULL;

static void
luau_sdump(lua_State *pL)
{
	fprintf(stderr, "---------- STACK DUMP ----------\n");
	for (int i = lua_gettop(pL); i; --i) {
		int t = lua_type(pL, i);
		switch (t) {
		break; case LUA_TSTRING:
			fprintf(stderr, "%4d: [string] '%s'\n", i, lua_tostring(pL, i));
		break; case LUA_TBOOLEAN:
			fprintf(stderr, "%4d: [bool]   '%s'\n", i,
				lua_toboolean(pL, i) ? "true" : "false");
		break; case LUA_TNUMBER:
			fprintf(stderr, "%4d: [number] '%g'\n", i, lua_tonumber(pL, i));
		break; case LUA_TNIL:
			fprintf(stderr, "%4d: [nil]\n", i);
		break; default:
			fprintf(stderr, "%4d: [%s] #%d <%p>\n", i, lua_typename(pL, t),
				(int) luau_rawlen(pL, i), lua_topointer(pL, i));
		break;
		}
	}
	fprintf(stderr, "---------- STACK  END ----------\n");
}

static int
luau_panic(lua_State *pL)
{
	char *err = (char *)lua_tostring(pL, -1);
	fprintf(stderr, "Lua error: %s\n", err);

	fputc('\n', stderr);
	luau_sdump(pL);
	fputc('\n', stderr);

	luaL_traceback(pL, pL, err, 0);
	fprintf(stderr, "Lua traceback: %s\n\n", lua_tostring(pL, -1));

	errx(1, "Fatal Lua error.");

	return 0;
}

static void
luau_init(lua_State **pL)
{
        *pL = luaL_newstate();
        assert(*pL);

        luaL_openlibs(*pL);
        luaopen_table(*pL);
        luaopen_string(*pL);
        luaopen_math(*pL);
        luaopen_io(*pL);

        lua_atpanic(*pL, luau_panic);
}

static void
luau_call(lua_State *pL, const char *namespace, const char *fnname, size_t nargs, size_t nret)
{
	/* get function from namespace. */
	if (namespace) {
		lua_getglobal(pL, namespace);
		lua_getfield(pL, -1, fnname);
		lua_remove(pL, -2);
	} else {
		lua_getglobal(pL, fnname);
	}

	/* move function before args. */
	lua_insert(pL, -nargs - 1);

	if (lua_pcall(pL, nargs, nret, -nargs - 1) == LUA_ERRERR) {
		luau_panic(pL);
	}
}

static int
fake_pclose(lua_State *pL)
{
	lua_pop(pL, -1);
	return 0;
}
