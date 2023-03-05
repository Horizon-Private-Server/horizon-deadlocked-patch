#include <tamtypes.h>
#include <sifrpc.h>
#include <libdl/stdio.h>
#include <libdl/net.h>

/* GS macro */
#define GS_BGCOLOUR	*((vu32*)0x120000e0)

int load_elf_ram(u32 loadFromAddress, u32 size);

/* Debug colors. Now NTSC Safe! At least I believe they are... */
int red = 0x1010B4; /* RED: Opening elf */
int green = 0x10B410; /* GREEN: Reading elf */
int blue = 0xB41010; /* BLUE: Installing elf header */
int yellow = 0x10B4B4; /* YELLOW: ExecPS2 */
int orange = 0x0049E6; /* ORANGE: SifLoadElf */
int pink = 0xB410B4; /* PINK: Launching OSDSYS */
int white = 0xEBEBEB; /* WHITE: Failed to launch OSDSYS */
int purple = 0x801080; /* PURPLE: Launching uLaunchELF */

/* ELF-header structures and identifiers */
#define ELF_MAGIC	0x464c457f
#define ELF_PT_LOAD	1

typedef struct {
	u8	ident[16];
	u16	type;
	u16	machine;
	u32	version;
	u32	entry;
	u32	phoff;
	u32	shoff;
	u32	flags;
	u16	ehsize;
	u16	phentsize;
	u16	phnum;
	u16	shentsize;
	u16	shnum;
	u16	shstrndx;
} elf_header_t;

typedef struct {
	u32	type;
	u32	offset;
	void *vaddr;
	u32	paddr;
	u32	filesz;
	u32	memsz;
	u32	flags;
	u32	align;
} elf_pheader_t;

/*
 * launch OSDSYS in case elf load failed (if fmcb installed, reloads it)
 */
void launch_OSDSYS(void)
{
	__asm__ __volatile__(
		"	li $3, 0x04;"
		"	syscall;"
		"	nop;"
	);
}

void ResetEE(int a0)
{
	__asm__ __volatile__(
		"	li $3, 0x01;"
		"	syscall;"
		"	nop;"
	);
}

void FlushCache(int a0)
{
	__asm__ __volatile__(
		"	li $3, 0x64;"
		"	syscall;"
		"	nop;"
	);
}

void ExecPS2(u32* a0, u32 a1, u32 a2, u32 a3)
{
	__asm__ __volatile__(
		"	li $3, 0x07;"
		"	syscall;"
		"	nop;"
	);
}

void DisableIntc(int intc)
{
	__asm__ __volatile__(
		"	li $3, 0x15;"
		"	syscall;"
		"	nop;"
	);
}

void RemoveIntcHandler(int intc, int callbackId)
{
	__asm__ __volatile__(
		"	li $3, 0x11;"
		"	syscall;"
		"	nop;"
	);
}

int DIntr()
{
    int eie, res;

    asm volatile("mfc0\t%0, $12"
                 : "=r"(eie));
    eie &= 0x10000;
    res = eie != 0;

    if (!eie)
        return 0;

    asm(".p2align 3");
    do {
        asm volatile("di");
        asm volatile("sync.p");
        asm volatile("mfc0\t%0, $12"
                     : "=r"(eie));
        eie &= 0x10000;
    } while (eie);

    return res;
}

int EIntr()
{
    int eie;

    asm volatile("mfc0\t%0, $12"
                 : "=r"(eie));
    eie &= 0x10000;
    asm volatile("ei");

    return eie != 0;
}

void clear(void* dest, int size)
{
  while (size > 0)
  {
    if (size > 64) {
      __asm__ (
        "\tsq $0, 0(%0) \n"
        "\tsq $0, 16(%0) \n"
        "\tsq $0, 32(%0) \n"
        "\tsq $0, 48(%0) \n"
        :: "r" (dest)
      );

      size -= 64;
      dest += 64;
    }
    else {
      *(u8*)dest = 0;
      size -= 1;
      dest += 1;
    }
  }
}

void copy(void* dest, void* src, int size)
{
  while (size > 0)
  {
    if (size > 64) {
      __asm__ (
        "\tlq $2, 0(%0) \n"
        "\tsq $2, 0(%1) \n"
        "\tlq $2, 16(%0) \n"
        "\tsq $2, 16(%1) \n"
        "\tlq $2, 32(%0) \n"
        "\tsq $2, 32(%1) \n"
        "\tlq $2, 48(%0) \n"
        "\tsq $2, 48(%1) \n"
        :: "r" (src), "r" (dest)
      );

      size -= 64;
      dest += 64;
      src += 64;
    }
    else {
      *(u8*)dest = *(u8*)src;
      size -= 1;
      dest += 1;
      src +=  1;
    }
  }
}

u32 _addr = 0, _size = 0;
void hook2()
{
  int i;

  // remove timer handlers
  DIntr();
  DisableIntc(11);
  RemoveIntcHandler(11, *(int*)0x00165D30);
  EIntr();

  // reset iop
	SifInitRpc(0);
  ((void (*)(char*, int))0x0012cc30)("", 0);
  while (!SifIopSync()) {};
	SifExitRpc();

	//Taken from Open PS2 Loader's elfldr.c
	ResetEE(0x7f);

	/* Clear user memory */
	for (i = 0x00100000; i < 0x02000000; i += 64) {
		__asm__ (
			"\tsq $0, 0(%0) \n"
			"\tsq $0, 16(%0) \n"
			"\tsq $0, 32(%0) \n"
			"\tsq $0, 48(%0) \n"
			:: "r" (i)
		);
	}

	/* Clear scratchpad memory */
	clear((void*)0x70000000, 16 * 1024);
	
	load_elf_ram(_addr, _size);

	//If that fails, then boot OSDSYS
	//GS_BGCOLOUR = pink; /* PINK: Launching OSDSYS */
  //SifLoadFileExit();
  //fioExit();
  //SifExitRpc();
  launch_OSDSYS();

 	GS_BGCOLOUR = white; /* WHITE: Failed to load OSDSYS */
  //SleepThread();
	//execute_elf(argv[0]);
}

/*
 * main function
 */
int loadelf (u32 loadFromAddress, u32 size)
{
	int i;
	
  _addr = loadFromAddress;
  _size = size;

  //
  void * connection = netGetLobbyServerConnection();
  ((void (*)(void**))0x0077A178)(&connection);

  // shut everything down
  //*(u32*)0x005cb848 = 0x0C000000 | ((u32)&hook2 >> 2);
  *(u32*)0x0012da18 = 0x08000000 | ((u32)&hook2 >> 2);
  ((void (*)(void))0x005c8458)();

	return -1;
}

int load_elf_ram(u32 loadFromAddress, u32 size) {
	int r;

  u32 elfloc = 0x01800000;
  elf_header_t *boot_header;
  elf_pheader_t *boot_pheader;
  int i;

  // move elf to temp location
  copy((void*)elfloc, (void*)loadFromAddress, size);

  GS_BGCOLOUR = red; /* RED: Opening elf */
  GS_BGCOLOUR = green; /* GREEN: Reading elf */
  GS_BGCOLOUR = blue; /* BLUE: Installing elf header */
  boot_header = (elf_header_t*)elfloc;

  /* Check elf magic */
  if ((*(u32*)boot_header->ident) != ELF_MAGIC)
    return -2;

  /* Get program headers */
  boot_pheader = (elf_pheader_t*)(elfloc + boot_header->phoff);

  /* Scan through the ELF's program headers and copy them into apropriate RAM
  * section, then padd with zeros if needed.
  */
  for (i = 0; i < boot_header->phnum; i++) {
    if (boot_pheader[i].type != ELF_PT_LOAD)
      continue;

    copy(boot_pheader[i].vaddr, (u8 *)elfloc + boot_pheader[i].offset, (int)(boot_pheader[i].filesz)); //(elf_pheader_t*)boot_pheader[i].filesz);

    if (boot_pheader[i].memsz > boot_pheader[i].filesz)
      clear(boot_pheader[i].vaddr + boot_pheader[i].filesz, boot_pheader[i].memsz - boot_pheader[i].filesz);
  }

  if (!boot_header->entry) //If the entrypoint is 0 exit
    return -3;

  //Booting part
  GS_BGCOLOUR = yellow; /* YELLOW: ExecPS2 */
  //SifLoadFileExit();
  //fioExit();

  //FlushCache(0);
  //FlushCache(2);
  
  //((void (*)(u32*, u32, u32, u32))0x001267f0)((u32*)boot_header->entry, 0, 0, NULL);
  char * args[] = { "" };
  ExecPS2((u32*)boot_header->entry, 0, 1, args);
}
