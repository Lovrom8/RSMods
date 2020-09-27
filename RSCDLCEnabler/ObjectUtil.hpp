#pragma once
#include <cstdint>
#include <vector>
#include "Offsets.hpp"

namespace ObjectUtil {
	struct Object {
		// uk stands for unknown mkay?
		uintptr_t vftable;	// 0x00

		uintptr_t uk04;		// 0x04
		uintptr_t uk08;		// 0x08
		uintptr_t uk0C;		// 0xC
		char* className;	// 0x10
		uintptr_t uk14;		// 0x14
		uintptr_t uk18;		// 0x18
		uintptr_t uk1c;		// 0x1C
		uintptr_t uk20;		// 0x20

		Object* parent;		// 0x24

		uintptr_t uk28;		// 0x28
		uintptr_t uk2c;		// 0x2c
		uintptr_t uk30;		// 0x30
		uintptr_t uk34;		// 0x34
		uintptr_t uk38;		// 0x38
		uintptr_t uk3c;		// 0x3C
		uintptr_t uk40;		// 0x40
		uintptr_t uk44;		// 0x44
		uintptr_t uk48;		// 0x48
		uintptr_t uk4C;		// 0x4C
		uintptr_t uk50;		// 0x50
		uintptr_t uk54;		// 0x54
		uintptr_t uk58;		// 0x58
		uintptr_t uk5C;		// 0x5C
		uintptr_t uk60;		// 0x60
		uintptr_t uk64;		// 0x64
		uintptr_t uk68;		// 0x68
		uintptr_t uk6C;		// 0x6C

		float z;			// 0x70
		float x;			// 0x74
		float y;			// 0x78

		float scale;		// 0x7C

		uintptr_t uk80;		// 0x80
		uintptr_t uk84;		// 0x84
		uintptr_t uk88;		// 0x88
		uintptr_t uk8C;		// 0x8C
		uintptr_t uk90;		// 0x90
		uintptr_t uk94;		// 0x94
		uintptr_t uk98;		// 0x98
		uintptr_t uk9c;		// 0x9C
		uintptr_t ukA0;		// 0xA0
		uintptr_t ukA4;		// 0xA4
		uintptr_t ukA8;		// 0xA8
		uintptr_t ukAC;		// 0xAC
		uintptr_t ukB0;		// 0xB0
		uintptr_t ukB4;		// 0xB4
		uintptr_t ukB8;		// 0xB8
		uintptr_t ukBC;		// 0xBC
		uintptr_t ukC0;		// 0xC0
		uintptr_t ukC4;		// 0xC4
		uintptr_t ukC8;		// 0xC8
		uintptr_t ukCC;		// 0xCC
		uintptr_t ukD0;		// 0xD0
		uintptr_t ukD4;		// 0xD4

		Object** children;	// 0xD8 array of pointers to Objects
		unsigned short childrenSize;// 0xDC
		unsigned short childCount;	// 0xDE
	};

	extern Object* GetRootObject();
	extern std::vector<Object*> GetChildrenOfObject(Object* parent);
}