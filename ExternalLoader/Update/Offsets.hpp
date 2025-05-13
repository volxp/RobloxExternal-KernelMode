#pragma once
#include <iostream>


namespace Offsets {
	inline uintptr_t name = 0x78;
	inline uintptr_t parent = 0x50;
	inline uintptr_t children = 0x80;
	inline uintptr_t size = 0x8; // childrenend
	inline uintptr_t datamodel = 0x0;
	inline uintptr_t value = 0xD8;
	inline uintptr_t localplr = 0x128; // "LocalPlayer"
	inline uintptr_t walkspeed = 0x1D8;
	inline uintptr_t jumppower = 0x1B8;
	inline uintptr_t walkspeedcheck = 0x3A8;
	inline uintptr_t gravity = 0x8C8; // Borken
	inline uintptr_t health = 0x19C;
	inline uintptr_t Fov = 0x168; // NOT IMPLEMENTED YET
	inline uintptr_t position = 0x140;





	inline uintptr_t DatamodelPtr = 0x62BB9F8;

}