# Scrapped pieces of code

## Team Colored Shots

This code changes the fusion shot color to be the same color as source player's team.

```cpp
u32 GetShotColor(Player* player)
{
	if (!player)
		return 0x80007F7F;

	return TEAM_COLORS[player->Team]; //colorLerp(TEAM_COLORS[player->Team], 0, 0.5);
}

u32 GetShotColor2(Moby* shot)
{
	Player * player = guberGetObjectByUID((u32)shot->NetObject);
	return GetShotColor(player);
}

void FusionShotDrawFunction(Moby* fusionShotMoby)
{
	u32 color = GetShotColor2(fusionShotMoby);
	color = colorLerp(color, 0, 0.25);
	u32 colorDark = color;
	u16 colorUpper = color >> 16;
	u16 colorLower = color & 0xFFFF;

	// change colors to team
	POKE_U32(0x003fd46c, 0x3C020000 | colorUpper);
	POKE_U16(0x003fd470, colorLower);
	POKE_U32(0x003fd8cc, 0x3C020000 | colorUpper);
	POKE_U16(0x003fd8d4, colorLower);

	POKE_U32(0x003fde68, 0x3C080000 | colorUpper);
	POKE_U32(0x003fdeb8, 0x3C080000 | colorUpper);
	POKE_U32(0x003fda44, 0x351E0000 | colorLower);
	POKE_U32(0x003fde6c, 0x3C090000 | (colorDark >> 16));
	POKE_U32(0x003fdebc, 0x3C090000 | (colorDark >> 16));
	POKE_U32(0x003fda4c, 0x35370000 | (u16)colorDark);

	// call base
	((void (*)(Moby*))0x003fdcb0)(fusionShotMoby);
}

void writeDrawFunctionPatch(u32 addr1, u32 addr2, u32 value)
{
	POKE_U16(addr1, (u16)(value >> 16));
	POKE_U16(addr2, (u16)value);
}
```
