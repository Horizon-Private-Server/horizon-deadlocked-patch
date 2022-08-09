#include <libdl/game.h>
#include <libdl/ui.h>
#include <libdl/pad.h>
#include <libdl/string.h>
#include <libdl/utils.h>

#define SERVER_HOSTNAME               ((char*)0x004BF4F0)
#define SERVER_SWITCH_LAST_VALUE     	(*(u8*)0x000CFFF4)
#define MAX_UNIVERSES									(4)
#define UNIVERSE_STORAGE_ADDRESS			(0x000B0000)

void getSelectedUniverseOffset(u32 stack)
{
	int universeCount = 0;
	char* universeNames[MAX_UNIVERSES];
	u32 universesPtr = UNIVERSE_STORAGE_ADDRESS;

	while (universeCount < MAX_UNIVERSES)
	{
		// store ptr to universe name and increment counter
		universeNames[universeCount] = (char*)(universesPtr + 0x24);
		universeCount++;

		// check if last universe
		if (*(char*)(universesPtr + 0x3BC))
			break;

		universesPtr += 0x3C0;
	}

	// only prompt for selection if more than one universe
	int selected = 0;
	if (universeCount > 1) {
		selected = uiShowSelectDialog("Select Server", universeNames, universeCount, SERVER_SWITCH_LAST_VALUE);
		if (selected < 0)
			selected = 0;
	}

	// save selected for next time
	SERVER_SWITCH_LAST_VALUE = (u8)selected;

	// copy selected universe to first in output list
	memcpy((void*)(stack + 0x80), (void*)(UNIVERSE_STORAGE_ADDRESS + 0x3C0 * selected), 0x3C0);
	if (selected > 0) {
		memcpy((void*)UNIVERSE_STORAGE_ADDRESS, (void*)(UNIVERSE_STORAGE_ADDRESS + 0x3C0 * selected), 0x3C0);
	}

	// to make sure program flow returns as normal
	// we set a0 back to the input a0
	asm volatile (
		"move $a0, %0"
		: : "r" (stack)
	);
}

/*
 * NAME :		main
 * 
 * DESCRIPTION :
 * 			Entrypoint.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int main (void)
{
	//POKE_U16(0x007555f0, -0x10D0);
	POKE_U32(0x007556B4, 0x3C050000 | (UNIVERSE_STORAGE_ADDRESS >> 16));
	POKE_U16(0x007556c4, MAX_UNIVERSES);
	HOOK_JAL(0x00755790, &getSelectedUniverseOffset);
	//POKE_U32(0x00755790, 0x27A40080);
	//POKE_U32(0x00755798, 0x0040802D);
  
	return 0;
}
