#pragma once
#include <iostream>

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
	#include "lstate.h"
}

class luaStateClass {
public:
	GCObject* next;
	lu_byte tt;
	lu_byte marked;
	lu_byte status;
	StkId top;  /* first free slot in the stack */
	StkId base;  /* base of current function */
	global_State* l_G;
	CallInfo* ci;  /* call info for current function */
	const Instruction* savedpc;  /* `savedpc' of current function */
	StkId stack_last;  /* last free slot in the stack */
	StkId stack;  /* stack base */
	CallInfo* end_ci;  /* points after end of ci array*/
	CallInfo* base_ci;  /* array of CallInfo's */
	int stacksize;
	int size_ci;  /* size of array `base_ci' */
	unsigned short nCcalls;  /* number of nested C calls */
	unsigned short baseCcalls;  /* nested C calls when resuming coroutine */
	lu_byte hookmask;
	lu_byte allowhook;
	int basehookcount;
	int hookcount;
	lua_Hook hook;
	TValue l_gt;  /* table of globals */
	TValue env;  /* temporary place for environments */
	GCObject* openupval;  /* list of open upvalues in this stack */
	GCObject* gclist;
	struct lua_longjmp* errorJmp;  /* current error recover point */
	ptrdiff_t errfunc;  /* current error handling function (stack index) */
};

extern void DumpLuaStateToConsole(luaStateClass luaStateToConsole);
extern void printTostdCoutExternal(lua_State* luaState, std::string printMessage);