#include "Lua/Lua.hpp"

int printTostdCout(lua_State* luaState) {
    int nargs = lua_gettop(luaState);
    std::cout << "Converting Print To Cout" << std::endl;
    for (int i = 1; i <= nargs; ++i) {
        std::cout << lua_tostring(luaState, i);
    }
    std::cout << " " <<std::endl;

    return 0;
}

const struct luaL_Reg printlib[] = {
    {"print", printTostdCout},
    {NULL, NULL}
};

void printTostdCoutExternal(lua_State* luaState, std::string printMessage) {
    luaL_openlibs(luaState);
    lua_getglobal(luaState, "_G");
    luaL_register(luaState, NULL, printlib);
    lua_pop(luaState, 1);
    luaL_dostring(luaState, printMessage.c_str());
}

void DumpLuaStateToConsole(luaStateClass luaStateToConsole) {
	std::cout << "GCObject Next: " << luaStateToConsole.next << std::endl;
	std::cout << "TT: " << (unsigned int)luaStateToConsole.tt << std::endl;
	std::cout << "Marked: " << (unsigned int)luaStateToConsole.marked << std::endl;

	std::cout << "Status: " << (unsigned int)luaStateToConsole.status << std::endl;
	std::cout << "Base: " << luaStateToConsole.base << std::endl;
	std::cout << "Top: " << luaStateToConsole.top << std::endl;

	std::cout << "SavedPC: " << luaStateToConsole.savedpc << std::endl;

	std::cout << "Stack: " << luaStateToConsole.stack << std::endl;
	std::cout << "Stack Size: " << luaStateToConsole.stacksize << std::endl;
	std::cout << "Stack Last: " << luaStateToConsole.stack_last << std::endl;

	std::cout << "Base Ci: " << luaStateToConsole.base_ci << std::endl;
	std::cout << "Ci: " << luaStateToConsole.ci << std::endl;
	std::cout << "End Ci:" << luaStateToConsole.end_ci << std::endl;
	std::cout << "Size Ci:" << luaStateToConsole.size_ci << std::endl;

	std::cout << "N Ccalls: " << luaStateToConsole.nCcalls << std::endl;
	std::cout << "Base Ccalls: " << luaStateToConsole.baseCcalls << std::endl;

	std::cout << "Hook Mask: " << (unsigned int)luaStateToConsole.hookmask << std::endl;
	std::cout << "Allow Hook: " << (unsigned int)luaStateToConsole.allowhook << std::endl;
	std::cout << "Base Hook Count: " << luaStateToConsole.basehookcount << std::endl;
	std::cout << "Hook Count: " << luaStateToConsole.hookcount << std::endl;
	std::cout << "Hook: " << luaStateToConsole.hook << std::endl;

	std::cout << "l_G: " << luaStateToConsole.l_G << std::endl;
	std::cout << "l_gt TT: " << luaStateToConsole.l_gt.tt << std::endl;
	std::cout << "l_gt GC: " << luaStateToConsole.l_gt.value.gc << std::endl;
	std::cout << "l_gt B: " << luaStateToConsole.l_gt.value.b << std::endl;
	std::cout << "l_gt N: " << luaStateToConsole.l_gt.value.n << std::endl;
	std::cout << "l_gt P: " << luaStateToConsole.l_gt.value.p << std::endl;

	std::cout << "env TT: " << luaStateToConsole.env.tt << std::endl;
	std::cout << "env GC: " << luaStateToConsole.env.value.gc << std::endl;
	std::cout << "env B: " << luaStateToConsole.env.value.b << std::endl;
	std::cout << "env N: " << luaStateToConsole.env.value.n << std::endl;
	std::cout << "env P: " << luaStateToConsole.env.value.p << std::endl;

	std::cout << "Open Up Value:  " << luaStateToConsole.openupval << std::endl;
	std::cout << "GCList: " << luaStateToConsole.gclist << std::endl;

	std::cout << "Error Jump: " << luaStateToConsole.errorJmp << std::endl;
	std::cout << "Error Function: " << luaStateToConsole.errfunc << std::endl;
};