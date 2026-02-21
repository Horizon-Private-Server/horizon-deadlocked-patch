/*
	CodeDesigner v2.0
	Created by: Gtlcpimp
	Official CheatersLounge Product Copyright ©
*/

/*
   To hook the exception handler, use the kernel hook command
   '9' as a code to launch the _installHandler function

   Code:
   900E0000 00000000

   Do not joker it.

  ------------------------
  If not using RemotePS2, just call the _installHandler function
  from anywhere after entering kernel mode.
*/

/*
I, Dnawrkshp, removed the install handler so that I could call it from the LDHook instead of having to hook something to the installHandler in the EE.

Now I just removed it entirely as it doesn't support dynamic allocation
*/


address $000C8000


//==============================================================
_installHandler:
addiu sp, sp, $fff0
sq ra, $0000(sp)

jal :DI
nop
jal :ee_kmode_enter
nop

lui a0, $8000
addiu a1, zero, $8f5a

_IH_ScanLoop:

lh v0, $0000(a0)
lh v1, $0002(a0)
bne a1, v1, :_IH_ScanLoop
addiu a0, a0, 4

lui a0, $8001
daddu a0, a0, v0

setreg v0, :_ExceptionHandler

sw v0, $0004(a0) // TLB Modification
sw v0, $0008(a0) // TLB Load/Inst Fetch
sw v0, $000c(a0) // TLB Store
sw v0, $0010(a0) // Address Load/Inst Fetch
sw v0, $0014(a0) // Address Store
sw v0, $0018(a0) // Bus Error (instr)
sw v0, $001c(a0) // Bus Error (data)
//sw v0, $0020(a0) // Syscall()
//sw v0, $0024(a0) // Breakpoint
sw v0, $0028(a0) // Reserved Instruction
sw v0, $002c(a0) // Coprocessor Unsuable
sw v0, $0030(a0) // Overflow
sw v0, $0034(a0) // Something

jal :ee_kmode_exit
nop
jal :EI
nop

ld ra, $0000(sp)
jr ra
addiu sp, sp, $0010

//==============================================================
_ExceptionHandler:

//--------------------------- Dump Registers
addiu sp, sp, $FFF0
sq k0, $0000(sp)

setreg k0, :_regSave
addiu k0, k0, 4

sw at, $0000(k0)
sw v0, $0004(k0)
sw v1, $0008(k0)
sw a0, $000c(k0)
sw a1, $0010(k0)
sw a2, $0014(k0)
sw a3, $0018(k0)
sw t0, $001c(k0)
sw t1, $0020(k0)
sw t2, $0024(k0)
sw t3, $0028(k0)
sw t4, $002c(k0)
sw t5, $0030(k0)
sw t6, $0034(k0)
sw t7, $0038(k0)
sw s0, $003c(k0)
sw s1, $0040(k0)
sw s2, $0044(k0)
sw s3, $0048(k0)
sw s4, $004c(k0)
sw s5, $0050(k0)
sw s6, $0054(k0)
sw s7, $0058(k0)
sw t8, $005c(k0)
sw t9, $0060(k0)
sw gp, $0064(k0)
sw fp, $0068(k0)

addiu at, sp, $0010
sw at, $006c(k0)
sw k1, $0074(k0)

lq k1, $0000(sp)
sw k1, $0070(k0)

sw ra, $0078(k0)

mfc0 v0, $Cause
sw v0, $0080(k0)

mfc0 v0, $EPC
sw v0, $0084(k0)

mfc0 v0, $BadVAddr
sw v0, $0088(k0)

mfc0 v0, $Status
sw v0, $008c(k0)

addiu sp, sp, $0010

//--------------------------- Exit Kernel To Exception Screen

setreg v0, :_ExceptionDisplay
mtc0 v0, $EPC
sync.p
eret
nop

//==============================================================
_ExceptionDisplay:

addiu sp, sp, $FF00
sq ra, $0000(sp)
sq s0, $0010(sp)
sq s1, $0020(sp)
sq s2, $0030(sp)
sq s3, $0040(sp)
sq s4, $0050(sp)
sq s5, $0060(sp)

lui s1, $0009

jal :_GetDisplay
nop
sd v0, $00F0(sp)
sd v1, $00F8(sp)

jal :_SetDisplay
nop

addiu s0, zero, 10

_initForceLoop:

addiu a1, zero, $30
addiu a2, zero, $30
addiu a3, zero, $A0
jal :_initScreen
daddu a0, s1, zero

jal :vSync
nop

setreg a0, :_tempSpace
jal :_memClear
addiu a1, zero, 300

addiu a2, sp, $00F0
addiu v0, zero, 10
subu v0, v0, s0
sw v0, $0000(a2)

setreg a0, :_forcedGPU_str
setreg a1, :_tempSpace
jal :sprintf
addiu a2, sp, $00F0

daddu a0, s1, zero
addiu a1, zero, 5
addiu a2, zero, 5
daddu a3, zero, zero
addiu t0, zero, $ff
addiu t1, zero, $ff
addiu t2, zero, $ff
daddu t3, zero, zero
addiu t4, zero, 2
addiu t5, zero, 0
setreg t6, :_tempSpace
jal :FontChar_DrawString
nop

jal :vSync
nop

bne s0, zero, :_initForceLoop
addiu s0, s0, -1


addiu a1, zero, $30
addiu a2, zero, $30
addiu a3, zero, $A0
jal :_ClearBuffer
daddu a0, s1, zero


addiu s0, zero, 400

//------------------------------------------------- Frame Loop
_FrameLoop:

setreg s5, :_regSave

addiu s3, zero, 92
addiu s4, zero, 95
daddu s2, zero, zero



setreg a0, :_tempSpace
jal :_memClear
addiu a1, zero, 300

lw t0, $0084(s5)
lw t1, $0088(s5)
lw t2, $008C(s5)
lw t3, $0090(s5)
addiu a2, sp, $00F0
sw t0, $0000(a2)
sw t1, $0004(a2)
sw t2, $0008(a2)
sw t3, $000C(a2)

setreg a0, :_headerSetup
setreg a1, :_tempSpace
jal :sprintf
addiu a2, sp, $00F0

daddu a0, s1, zero
addiu a1, zero, 5
addiu a2, zero, 5
daddu a3, zero, zero
addiu t0, zero, $ff
addiu t1, zero, $ff
addiu t2, zero, $ff
daddu t3, zero, zero
addiu t4, zero, 2
addiu t5, zero, 0
setreg t6, :_tempSpace
jal :FontChar_DrawString
nop

daddu a0, s1, zero
addiu a1, zero, 86
addiu a2, zero, 410
daddu a3, zero, zero
addiu t0, zero, $ff
addiu t1, zero, $ff
addiu t2, zero, $ff
daddu t3, zero, zero
addiu t4, zero, 2
addiu t5, zero, 0
setreg t6, :_exceptionStr
jal :FontChar_DrawString
nop


//--------------------------------------- Reg Loop
_FrameRegisterLoop:

addiu v0, zero, 16
bne s2, v0, 3
nop
addiu s3, zero, 92
addiu s4, s4, 252


setreg a0, :_tempSpace
jal :_memClear
addiu a1, zero, 100

jal :_getRegName
daddu a0, s2, zero


addiu a2, sp, $00F0
sw v0, $0000(a2)


lw v0, $0000(s5)
sw v0, $0004(a2)
addiu s5, s5, 4

setreg a0, :_registerDump
setreg a1, :_tempSpace
jal :sprintf
addiu a2, sp, $00F0

setreg a0, :_tempSpace
jal :LCase
nop

daddu a0, s1, zero
daddu a1, s4, zero
daddu a2, s3, zero
daddu a3, zero, zero
addiu t0, zero, $ff
addiu t1, zero, $ff
addiu t2, zero, $ff
daddu t3, s2, zero
addiu t4, zero, 2
addiu t5, zero, 0
setreg t6, :_tempSpace
jal :FontChar_DrawString
nop

addiu s3, s3, 18
addiu s2, s2, 1
addiu v0, zero, 32
bne s2, v0, :_FrameRegisterLoop
nop
//--------------------------------------- Reg Loop Back

jal :vSync
nop

//addiu a1, zero, $30
//addiu a2, zero, $30
//addiu a3, zero, $A0
//jal :_ClearBuffer
//daddu a0, s1, zero

beq zero, zero, :_FrameLoop
nop
//bne s0, zero, :_FrameLoop
//addiu s0, s0, -1

ld a0, $00F0(sp)
jal :_RestoreDisplay
ld a1, $00F8(sp)

lq ra, $0000(sp)
lq s0, $0010(sp)
lq s1, $0020(sp)
lq s2, $0030(sp)
lq s3, $0040(sp)
lq s4, $0050(sp)
lq s5, $0060(sp)
jr ra
addiu sp, sp, $0100

//==============================================================
// Tiny Font Character To Pixels
/*
Input:
   a0 = &Packet
   a1 = X
   a2 = Y
   a3 = Z
   t0 = R
   t1 = G
   t2 = B
   t3 = Packet Size
   t4 = Font Size
   t5 = Bold
   t6 = &String
*/
FontChar_DrawString:

addiu sp, sp, $FF00
sq ra, $0000(sp)
sq s0, $0010(sp)
sq s1, $0020(sp)
sq s2, $0030(sp)
sq s3, $0040(sp)
sq s4, $0050(sp)
sq s5, $0060(sp)
sq s6, $0070(sp)
sq s7, $0080(sp)

daddu s0, a0, zero

sw a1, $0090(sp)
sw a2, $0094(sp)
sw a3, $0098(sp)
sb t0, $0099(sp)
sb t1, $009a(sp)
sb t2, $009b(sp)
sw t3, $009c(sp)
sb t4, $00A0(sp)
sb t5, $00A1(sp)

daddu s1, t6, zero

daddu s2, zero, zero
daddu s3, zero, zero

addiu s4, zero, 17

sw zero, $00B0(sp)

FontChar_DrawString_StrLoop:
lb t6, $0000(s1)
beq t6, zero, :FontChar_DrawString_StrLoop_Exit
nop
addiu v0, zero, $0A
bne t6, v0, 5
nop
daddu s2, zero, zero
addiu s3, s3, 12
beq zero, zero, :FontChar_DrawString_StrLoop_NextChar
nop

daddu a0, s0, zero
lw a1, $0090(sp)
lw a2, $0094(sp)

lb t4, $00A0(sp)
dsll32 t4, t4, 28
dsrl32 t4, t4, 28

multu v0, s2, t4
addu a1, a1, v0

multu v0, s3, t4
addu a2, a2, v0

lb t0, $0099(sp)
lb t1, $009a(sp)
lb t2, $009b(sp)
lw t3, $00B0(sp)
lb t4, $00A0(sp)
lb t5, $00A1(sp)
jal :FontChar_AddToPacket
lb t6, $0000(s1)

lw t3, $00B0(sp)
addu t3, t3, v0
sw t3, $00B0(sp)

addiu s2, s2, 9
addiu s4, s4, -1
bne s4, zero, :FontChar_DrawString_StrLoop_NextChar
nop

daddu a0, s0, zero
jal :_DMA_2_Upload
lw a1, $00B0(sp)

addiu s4, zero, 17
sw zero, $00B0(sp)



FontChar_DrawString_StrLoop_NextChar:
beq zero, zero, :FontChar_DrawString_StrLoop
addiu s1, s1, 1
FontChar_DrawString_StrLoop_Exit:

addiu v0, zero, 17
beq s4, v0, :FontChar_DrawString_Exit
nop
beq s4, zero, :FontChar_DrawString_Exit
nop
daddu a0, s0, zero
jal :_DMA_2_Upload
lw a1, $00B0(sp)


FontChar_DrawString_Exit:
lq ra, $0000(sp)
lq s0, $0010(sp)
lq s1, $0020(sp)
lq s2, $0030(sp)
lq s3, $0040(sp)
lq s4, $0050(sp)
lq s5, $0060(sp)
lq s6, $0070(sp)
lq s7, $0080(sp)
jr ra
addiu sp, sp, $0100

//==============================================================
// Tiny Font Character To Pixels
/*
Input:
   a0 = &Packet
   a1 = X
   a2 = Y
   a3 = Z
   t0 = R
   t1 = G
   t2 = B
   t3 = Packet Size
   t4 = Font Size
   t5 = Bold
   t6 = Char Ascii (eg. 'a')
Output:
   v0 = Size
*/
FontChar_AddToPacket:

addiu sp, sp, $FE00
sq ra, $0000(sp)
sq s0, $0010(sp)
sq s1, $0020(sp)
sq s2, $0030(sp)
sq s3, $0040(sp)
sq s4, $0050(sp)
sq s5, $0060(sp)
sq s6, $0070(sp)
sq s7, $0080(sp)

daddu s0, a0, zero

sw a1, $0090(sp)
sw a2, $0094(sp)
sw a3, $0098(sp)
sb t0, $009c(sp)
sb t1, $009d(sp)
sb t2, $009e(sp)
sw t3, $00A0(sp)
sb t4, $00A4(sp)
sb t5, $00A5(sp)
sb t6, $00A6(sp)
sw zero, $00B0(sp)

dsll32 a0, t6, 24
dsrl32 a0, a0, 24
jal :FontChar_Decompress
addiu a1, sp, $0100


daddu s1, zero, zero
daddu s2, zero, zero
addiu s3, sp, $0100

//----------------------------------------- Y Loop
_FontChar_AddPixel_Loop_Y:
lw s4, $0000(s3)
dsll32 s4, s4, 0
dsrl32 s4, s4, 0

daddu s2, zero, zero
//----------------------------------------- X Loop
_FontChar_AddPixel_Loop_X:

dsll32 s5, s4, 28
dsrl32 s5, s5, 28

beq s5, zero, :_FontChar_AddPixel_TransparentSkip
nop

daddu a0, s0, zero

lw a1, $0090(sp)
lw a2, $0094(sp)

lw a3, $0098(sp)

lb t0, $009c(sp)
lb t1, $009d(sp)
lb t2, $009e(sp)
dsll32 t0, t0, 24
dsll32 t1, t1, 24
dsll32 t2, t2, 24
dsrl32 t0, t0, 24
dsrl32 t1, t1, 24
dsrl32 t2, t2, 24

lw t3, $00A0(sp)

lb v0, $00A5(sp)
dsll32 v0, v0, 28
dsrl32 v0, v0, 28

addiu t4, zero, 1
addiu t5, zero, 1

beq v0, zero, 3
nop
addu t4, t4, v0
addu t5, t5, v0


lb v0, $00A4(sp)
dsll32 v0, v0, 28
dsrl32 v0, v0, 28

multu v1, s2, v0
addu a1, a1, v1

multu v1, s1, v0
addu a2, a2, v1

multu t4, t4, v0
multu t5, t5, v0

jal :_AddPixel
nop

nop

lw t3, $00A0(sp)
addu t3, t3, v0
sw t3, $00A0(sp)
lw v1, $00B0(sp)
addu v1, v1, v0
sw v1, $00B0(sp)

_FontChar_AddPixel_TransparentSkip:

//----------------------------------------- X Loop Back
srl s4, s4, 4
addiu v0, zero, 7
bne s2, v0, :_FontChar_AddPixel_Loop_X
addiu s2, s2, 1

//----------------------------------------- Y Loop Back
addiu s3, s3, 4
addiu v0, zero, 10
bne s1, v0, :_FontChar_AddPixel_Loop_Y
addiu s1, s1, 1


lw v0, $00B0(sp)

lq ra, $0000(sp)
lq s0, $0010(sp)
lq s1, $0020(sp)
lq s2, $0030(sp)
lq s3, $0040(sp)
lq s4, $0050(sp)
lq s5, $0060(sp)
lq s6, $0070(sp)
lq s7, $0080(sp)
jr ra
addiu sp, sp, $0200

//============================================================
// MemClear(&addr, size)

_memClear:
sb zero, $0000(a0)
addiu a0, a0, 1
bne a1, zero, :_memClear
addiu a1, a1, -1

jr ra
nop

//============================================================
// GetRegname
_getRegName:

sll a0, a0, 2
setreg v0, :_regName
jr ra
addu v0, v0, a0

//==============================================================
// String Formats

_forcedGPU_str:
print "Forcing initialization [%d]..."
nop

_headerSetup:
print "Cause: %x EPC: %x \nBadVAddr: %x Status: %x\n     -==== Register Dump ====-"
nop

_exceptionStr:
print "A fatal error has occured."
nop

_registerDump:
print "%s %x"
nop

_regName:
print "zr"
print "at"
print "v0"
print "v1"
print "a0"
print "a1"
print "a2"
print "a3"
print "t0"
print "t1"
print "t2"
print "t3"
print "t4"
print "t5"
print "t6"
print "t7"
print "s0"
print "s1"
print "s2"
print "s3"
print "s4"
print "s5"
print "s6"
print "s7"
print "t8"
print "t9"
print "gp"
print "fp"
print "sp"
print "k0"
print "k1"
print "ra"
print "??"
nop

//==============================================================
// Register Dump Zone
_regSave:
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop

//==============================================================
// File Imports

import "Classes\Kernel.cds"

import "Classes\Pixel 2.0.cds"

import "Classes\String.cds"

import "Classes\Math_Convert.cds"

import "Classes\TinyFont.cds"

_tempSpace:

