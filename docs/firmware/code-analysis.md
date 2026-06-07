# Code Analysis — A Bare-Metal STM32 Walkthrough

This document is both a **learning guide** and a **living reference** for the
firmware in this repository. It assumes you are fluent in C and comfortable with
hardware at the silicon/datasheet level (you've done PIC and semiconductor work),
but that **ARM Cortex-M and STM32 are new to you**. So nothing about the ARM
architecture, the STM32 peripheral model, or the GNU embedded toolchain is taken
for granted.

The goal is not to restate what each line does — it is to explain *why the project
is shaped the way it is*, what every file contributes to the build, and how the
pieces connect from power-on to a voltage on a DAC pin.

The firmware is small on purpose. The whole application is:

```
startup_stm32f407.s   →  Reset_Handler  →  main()
                                              ├── LED on PD12 (heartbeat)
                                              └── dac_init() / dac_write()
                                                    └── mcp4922_command()  (pure)
                                                    └── note_to_dac()      (pure)
                                                    └── spi2_init/write16  (register I/O)
```

---

## 1. Project Structure and Build System

### 1.1 Directory and file-type map

| Path | Type | Role |
|---|---|---|
| `Makefile` | build script | Drives the whole toolchain (compile, link, objcopy, flash, test). |
| `ld/stm32f407.ld` | linker script | Describes the chip's memory map and where each section of the program lands. |
| `startup/startup_stm32f407.s` | ARM assembly | The vector table and the `Reset_Handler` — the very first code that runs. |
| `src/*.c` | C source | The application and drivers. Each `.c` is a *translation unit* compiled to one `.o`. |
| `include/*.h` | C headers | Public interfaces (declarations + constants) shared between translation units. |
| `test/test_cv.c` | host test | Runs on your PC, not the chip; verifies the pure math/packing logic. |
| `build/` | artifacts | Generated `.o`, `.elf`, `.bin`, `.map`. Not checked in (see `.gitignore`). |
| `docs/`, `mkdocs.yml` | documentation | The MkDocs site you are reading now. |
| `.github/workflows/test.yml` | CI | Runs `make test` on every push/PR. |

The split between `src/` and `include/` is the conventional C separation of
**implementation** from **interface**. The split between `startup/` + `ld/` and
everything else is the embedded-specific part: on a PC the operating system and C
runtime provide the startup code and memory layout for you. On bare metal **there
is no OS** — you must supply both yourself, and that is exactly what those two
files are.

### 1.2 The Makefile

```makefile
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
HOSTCC ?= cc
```

Two compilers are in play, and understanding why is the key to this project's
build philosophy:

- **`arm-none-eabi-gcc`** is a *cross-compiler*. It runs on your PC but emits
  machine code for the ARM Cortex-M4 target. The triple `arm-none-eabi` means:
  ARM architecture, **no** OS (`none`), **e**mbedded **ABI**. There is no libc
  assuming a kernel, no `printf` to a console, no `malloc` backed by an OS heap.
- **`cc`** (`HOSTCC`) is your *native* compiler. It is used only for the unit
  tests, which run on the PC. `?=` means "use `cc` unless the environment already
  set `HOSTCC`," so CI or you can override it.

This dual-compiler setup is the single most important architectural idea in the
repo: **logic that is pure (no hardware) is compiled and tested natively; logic
that touches registers is cross-compiled and verified on the bench.** More on
that in §6.

#### The compile flags

```makefile
CFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
         -O0 -g -Wall -Wextra -ffreestanding -nostdlib -Iinclude
```

| Flag | Meaning | Why it's here |
|---|---|---|
| `-mcpu=cortex-m4` | Generate code for the Cortex-M4 core. | The STM32F407 *is* a Cortex-M4. This selects the legal instruction set. |
| `-mthumb` | Emit Thumb-2 instructions. | Cortex-M executes **only** Thumb-2; it has no ARM (32-bit) instruction mode at all. Omitting this would produce instructions the core cannot run. |
| `-mfloat-abi=hard` | Pass `float`/`double` in FPU registers and use FPU instructions. | The M4 on this chip has a single-precision FPU. `note_to_dac()` does `float` math; "hard" float makes it use the hardware unit instead of slow software emulation. |
| `-mfpu=fpv4-sp-d16` | Target the specific FPU (FPv4, single-precision, 16 D-registers). | Must match the silicon. Combined with `hard`, this is why the startup file has to *turn the FPU on* (see §1.4). |
| `-O0` | No optimization. | Bring-up phase: you want the disassembly to track the C line-for-line, and the busy-wait `delay()` must not be optimized away. Production would raise this. |
| `-g` | Emit debug info (DWARF). | Lets GDB/OpenOCD map addresses back to source. |
| `-Wall -Wextra` | Maximal warnings. | Cheap insurance on a platform where a silent bug means a soldering-iron debugging session. |
| `-ffreestanding` | Tell GCC there is **no hosted environment**. | A "hosted" C program may assume `main` has the usual semantics and the standard library exists. Freestanding says: only the language and a handful of headers (`stdint.h`, `stddef.h`…) are guaranteed. |
| `-nostdlib` | Don't link the standard library or default startup files. | On a PC, GCC silently links `crt0.o` (which calls `main`) and libc. We provide our **own** startup and our own (absent) runtime, so we must suppress the defaults or they'd collide with `Reset_Handler` and the linker script. |
| `-Iinclude` | Add `include/` to the header search path. | So `#include "cv.h"` resolves. See §2.5. |

#### The link flags

```makefile
LDFLAGS = -T ld/stm32f407.ld -Wl,--gc-sections -Wl,-Map=$(TARGET).map
```

- `-T ld/stm32f407.ld` — use **our** linker script instead of GCC's built-in
  default. On bare metal this is mandatory: the default script targets a Linux
  process address space, which is meaningless on the chip.
- `-Wl,--gc-sections` — "garbage-collect" unused sections. `-Wl,` passes the flag
  through `gcc` to the linker `ld`. Combined with the compiler's per-function
  sectioning, anything not reachable from the vector table or `main` is dropped,
  shrinking the binary. (This is why the linker script wraps the vector table in
  `KEEP()` — see §1.3 — so the collector doesn't delete the one section nothing
  appears to "call".)
- `-Wl,-Map=guitar-cv.map` — produce a map file showing the final address of
  every symbol and section. Indispensable for "did `.data` actually land where I
  think?" questions.

#### The targets

```makefile
all: $(TARGET).elf $(TARGET).bin
```

- **`all`** (default): build both the ELF and the raw binary.
- **`build/%.o: src/%.c | build`** and the matching `startup/%.s` rule: pattern
  rules that compile each source into `build/`. The `| build` is an *order-only
  prerequisite* — it ensures the `build/` directory exists first without forcing
  a rebuild every time the directory's timestamp changes.
- **`$(TARGET).elf`**: links all objects through the linker script into an ELF.
  The ELF contains the program plus symbol/debug metadata — it's what the
  debugger and `openocd ... program` consume.
- **`$(TARGET).bin`**: `objcopy -O binary` strips all the ELF metadata and emits
  the literal bytes that go into flash, starting at the lowest load address. Use
  this for a raw flash-writer or a bootloader that expects an image, not an ELF.
- **`flash`**: invokes OpenOCD with the ST-Link interface config and the
  STM32F4x target config, then `program ... verify reset exit` — write flash,
  read it back to verify, reset the chip, and quit. This is how the firmware gets
  onto the DISC1's onboard ST-Link.
- **`test`**: the host build. Note it compiles `src/cv.c src/mcp4922.c` together
  with `test/test_cv.c` using **`HOSTCC`** and runs the resulting executable. No
  ARM toolchain, no chip. This is what CI runs.
- **`clean`**: delete generated artifacts.
- **`.PHONY`**: declares targets that are *not* files, so Make never confuses a
  file named `clean` with the rule.

`SRC = $(wildcard src/*.c)` means **adding a new `.c` to `src/` needs no Makefile
edit** — it's picked up automatically. Good for a growing project.

### 1.3 The linker script (`ld/stm32f407.ld`)

**What a linker script is:** after the compiler turns each `.c` into an object
file full of code and data *sections*, the linker has to decide the final memory
address of every byte. A linker script is the description of (a) what memory the
chip physically has, and (b) which sections go into which memory. On a hosted OS,
the loader and a default script handle this. On bare metal, **you** own the memory
map, so you write it down explicitly.

```ld
ENTRY(Reset_Handler)
```

Declares the program's entry symbol. This doesn't set the hardware reset vector
(the CPU gets that from the vector table — §1.4); it tells the linker which symbol
to treat as the root for `--gc-sections` reachability, and records an entry point
in the ELF for debuggers.

```ld
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}
```

These two regions come **straight from the STM32F407 datasheet memory map**:

- **FLASH** at `0x08000000`, 1 MB. Attributes `rx` = readable, executable (you
  run code directly from flash; there's no "load into RAM first" step). It is
  *not* writable at runtime in the ordinary sense — it's non-volatile program
  storage.
- **SRAM** at `0x20000000`, 128 KB. `rwx` = read/write/execute. This is your
  volatile working memory: stack, globals, BSS.

> The Cortex-M architecture fixes these base addresses: `0x0800_0000` is the flash
> alias and `0x2000_0000` is the SRAM region, across the whole STM32 family. The
> *lengths* are part-specific (the F407 has 1 MB flash / 128 KB main SRAM, give or
> take an auxiliary 64 KB CCM block this script doesn't use).

```ld
_estack = ORIGIN(SRAM) + LENGTH(SRAM);   /* 0x20020000 — top of 128KB SRAM */
```

Defines the **initial stack pointer**: the top of SRAM. On Cortex-M the stack
grows *downward* (toward lower addresses), so it starts at the very top. This
symbol is consumed by the startup file's vector table (the first word the CPU
loads on reset is the stack pointer). `0x20000000 + 0x20000 = 0x20020000`. ✓

```ld
SECTIONS
{
    .text : { KEEP(*(.isr_vector)) *(.text*) *(.rodata*) . = ALIGN(4); } > FLASH
```

The `.text` output section, placed in FLASH, contains:

- `KEEP(*(.isr_vector))` — the vector table from the startup file, forced to the
  **start** of flash (because `.text` is the first section and `.isr_vector` is
  first within it). The CPU *requires* the vector table at the base of flash on
  reset, so its position here is not cosmetic — it's a hardware contract. `KEEP()`
  stops `--gc-sections` from deleting it (nothing "calls" the table, so the
  collector would otherwise consider it dead).
- `*(.text*)` — all code from all objects.
- `*(.rodata*)` — read-only data (string literals, `const` tables). It lives in
  flash because it never changes and flash is plentiful.
- `. = ALIGN(4)` — bump the location counter to a 4-byte boundary so the next
  section starts aligned (ARM loads/stores like alignment).

```ld
    .data : { _sdata = .; *(.data*) . = ALIGN(4); _edata = .; } > SRAM AT > FLASH
    _la_data = LOADADDR(.data);
```

This is the subtle one. **Initialized globals** (`int x = 5;`) present a paradox:
the *value* `5` must survive power-off, so it has to live in flash; but the
*variable* must be writable, so at runtime it must live in SRAM. The linker
solves it with two addresses:

- `> SRAM` — the **virtual** address (VMA): where the variable lives when the
  program runs (in SRAM).
- `AT > FLASH` — the **load** address (LMA): where the initial bytes are stored in
  the image (in flash).

The symbols `_sdata`/`_edata` mark the SRAM start/end of `.data`, and `_la_data =
LOADADDR(.data)` is the flash source address. The startup code uses all three to
**copy the initial values from flash to SRAM** before `main()`. Without that copy,
your initialized globals would be garbage.

```ld
    .bss : { _sbss = .; *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; } > SRAM
}
```

**Zero-initialized / uninitialized globals** (`int y;` or `int z = 0;`) don't need
any stored value — they just need to be zero at start. So `.bss` consumes *no
flash*; it only reserves an SRAM range marked by `_sbss`/`_ebss`. The startup code
writes zeros across that range. `*(COMMON)` catches tentative definitions.

So the four symbol pairs (`_sdata/_edata/_la_data`, `_sbss/_ebss`) and `_estack`
are the **entire contract** between the linker script and the startup file.

### 1.4 The startup file (`startup/startup_stm32f407.s`)

**What it is:** the first instructions that execute after reset, written in ARM
assembly because at that instant there is no stack, no initialized memory, and no
C runtime — none of C's assumptions hold yet. Its job is to build the environment
that C *needs*, then jump to `main`.

```asm
.syntax unified
.cpu cortex-m4
.thumb
```

Use modern unified assembly syntax, target the M4, assemble as Thumb (the only
mode the core supports).

#### The vector table

```asm
.section .isr_vector, "a"
.word _estack
.word Reset_Handler
.word Default_Handler   @ ×14
```

This is the **vector table** — an array of 32-bit words at the base of flash.
Cortex-M hardware defines its layout:

- **Word 0 = initial Main Stack Pointer.** On reset, the CPU literally loads this
  word into `SP` before executing a single instruction. That's why `_estack` (top
  of SRAM, from the linker script) goes here. This is unusual compared to other
  architectures — the stack is set up *by hardware*, from a table, not by code.
- **Word 1 = Reset vector.** The address the CPU jumps to out of reset. It points
  at `Reset_Handler`.
- **Words 2…15** = the system exceptions (NMI, HardFault, MemManage, BusFault,
  UsageFault, reserved slots, SVCall, DebugMon, PendSV, SysTick). Here they all
  point at `Default_Handler`, an infinite loop — a placeholder "if any fault
  fires, hang here so a debugger can catch it." A production build would give at
  least HardFault its own handler.

The `"a"` flag marks the section *allocatable* so the linker places it in the
image; `KEEP()` in the linker script keeps it from being garbage-collected.

#### Reset_Handler — what happens between power-on and main()

```asm
Reset_Handler:
    @ Enable FPU - set CP10 and CP11 full access
    ldr r0, =0xE000ED88        @ CPACR (Coprocessor Access Control Register)
    ldr r1, [r0]
    orr r1, r1, #(0xF << 20)   @ CP10/CP11 = full access (bits 23:20 = 1111)
    str r1, [r0]
    dsb
    isb
```

**Step 1 — turn on the FPU.** The Cortex-M4F powers up with the floating-point
unit *disabled*. But `CFLAGS` has `-mfloat-abi=hard`, so the compiler emits FPU
instructions (used by `note_to_dac`'s `float` math). If the FPU is off and an FPU
instruction executes, you get a UsageFault → HardFault → the `Default_Handler`
hang. So before any C runs, the code sets CP10 and CP11 (the two coprocessor
slots the FPU occupies) to full access in `CPACR` at `0xE000ED88`. `dsb`/`isb` are
barriers ensuring the write lands and the pipeline re-fetches before FPU
instructions are allowed to execute. **This is the #1 gotcha for newcomers doing
float math on an M4.**

```asm
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_la_data
copy_data:
    cmp r0, r1
    bge zero_bss
    ldr r3, [r2], #4           @ load from flash (LMA), post-increment
    str r3, [r0], #4           @ store to SRAM (VMA), post-increment
    b copy_data
```

**Step 2 — copy `.data` from flash to SRAM.** This is the runtime half of the
`AT > FLASH` trick. It walks from `_sdata` to `_edata` in SRAM, pulling source
words from `_la_data` in flash. After this loop, initialized globals hold their
intended values. (This program currently has no `.data`, so the loop runs zero
iterations — but the mechanism must exist for the moment one is added.)

```asm
zero_bss:
    ldr r0, =_sbss
    ldr r1, =_ebss
    mov r2, #0
zero_loop:
    cmp r0, r1
    bge call_main
    str r2, [r0], #4
    b zero_loop
```

**Step 3 — zero `.bss`.** Writes 0 across `_sbss`…`_ebss`. This is what makes the
C guarantee "uninitialized globals start at zero" true. Skip it and your `static`
counters start as whatever was in SRAM at power-on.

```asm
call_main:
    bl main
    b .
```

**Step 4 — call `main`.** `bl` (branch-with-link) into C. If `main` ever returns
(it shouldn't — it's an infinite `while(1)`), `b .` is a branch-to-self trap so
the CPU doesn't wander off into undefined memory.

```asm
Default_Handler:
    b .
```

The catch-all fault/IRQ handler: spin forever. In a debugger you'd see the PC
parked here and know a fault fired.

**How the three files interlock:** the linker script *defines* `_estack`,
`_sdata`, `_edata`, `_la_data`, `_sbss`, `_ebss`; the startup file *consumes* them
to build the vector table and initialize memory; the Makefile *links* them
together with `-nostdlib` so none of GCC's default startup interferes. Remove any
one and the chip either won't boot, won't have valid globals, or will fault on the
first `float`.

---

## 2. Header Files and the C Include System

### 2.1 What a `.h` file is — and is not

A header is **text that gets pasted into a `.c` file** by the preprocessor at the
`#include` line, before the compiler proper sees anything. That's the whole
mechanism. Critically:

- A header is **not a compilation unit.** It is never compiled on its own. There
  is no `cv.o` produced from `cv.h`. It only ever exists as part of whatever `.c`
  included it. (You can spot this in the Makefile: the `OBJ` list is built from
  `src/*.c` and `startup/*.s` — headers appear nowhere, because they produce no
  object file.)
- A header is **not where code lives** (with narrow exceptions like `static
  inline`). It is where you put *promises* about code that lives elsewhere.

### 2.2 Declaration vs definition

This distinction is the reason headers exist at all:

- A **declaration** says *"this thing exists somewhere, here's its type/signature."*
  Example, in `cv.h`:
  ```c
  uint16_t note_to_dac(uint8_t midi_note);
  ```
  This promises the linker there is a function with this name and signature. It
  allocates nothing and generates no code.
- A **definition** is the actual thing — the function body, the storage for a
  variable. Example, in `cv.c`:
  ```c
  uint16_t note_to_dac(uint8_t midi_note) { ... }
  ```

C's rule: a declaration may appear **many times** (every `.c` that calls the
function includes the header), but a definition must appear **exactly once**
across the whole program (the *One Definition Rule*). Headers carry declarations
precisely *because* they get duplicated into many translation units; definitions
stay in `.c` files precisely *because* they must not.

When `main.c` calls `note_to_dac()`, the compiler only needs the **declaration**
from `cv.h` to type-check the call and emit a placeholder. The **linker** later
resolves that placeholder to the definition compiled into `cv.o`. Header =
compile-time promise; `.o` = link-time fulfillment.

### 2.3 Include guards

Every header here opens with:

```c
#ifndef CV_H
#define CV_H
...
#endif /* CV_H */
```

This is an **include guard**. Because `#include` is literal text substitution, a
header can easily be pasted twice into one translation unit (e.g. `dac.c`
includes `dac.h` and `mcp4922.h`, and a future header might include another). The
second paste would re-declare things and, for anything that *can't* be repeated,
cause a compile error. The guard works by: the first inclusion defines `CV_H`;
any later inclusion sees `CV_H` already defined and the preprocessor skips the
body. The macro name is conventionally the filename uppercased with the dot turned
into an underscore.

### 2.4 The four headers in this project

| Header | Declares | Why it must be a header | What breaks without it |
|---|---|---|---|
| `cv.h` | `note_to_dac()` prototype; the calibration constants `CV_VREF`, `CV_DAC_COUNTS`, `CV_REF_NOTE`, `CV_SEMIS_OCT`, `CV_DAC_MAX`. | `main.c` and `test/test_cv.c` both call `note_to_dac` and must agree on its signature; the `#define`s are shared truth between the implementation (`cv.c`) and any future caller. | `main.c` would have no prototype → implicit-declaration error under `-Wall`; the host test couldn't see the constants; the conversion contract would be duplicated and could drift. |
| `mcp4922.h` | `mcp4922_command()` prototype; `MCP4922_CHANNEL_A/_B`. Plus a comment documenting the command-word bit layout. | `dac.c`, `main.c`, and the test all need the channel macros and the prototype. | `main.c`'s `MCP4922_CHANNEL_A` would be undefined; `dac.c` couldn't call `mcp4922_command`; the test couldn't link. |
| `spi2.h` | `spi2_init()`, `spi2_write16()` prototypes. | `dac.c` calls both; keeping the prototype here lets the SPI implementation change freely as long as the signatures hold. | `dac.c` would have no prototypes for the SPI driver → implicit declaration / link error. |
| `dac.h` | `dac_init()`, `dac_write()` prototypes — the DAC façade. | `main.c` programs against this small surface and stays ignorant of SPI/MCP details. | `main.c` couldn't see the DAC API. |

Notice the **layering** the headers encode: `main.c` includes `dac.h`,
`mcp4922.h`, `cv.h` but **not** `spi2.h`. The SPI driver is an implementation
detail hidden behind `dac.c`. That's a deliberate dependency boundary expressed
entirely through which headers each `.c` includes.

Also notice every header includes `<stdint.h>` for the fixed-width types
(`uint16_t`, `uint8_t`). A header must be **self-contained**: if it mentions
`uint16_t`, it must pull in the type itself rather than assume the includer did.

### 2.5 Angle brackets vs quotes

```c
#include <stdint.h>   /* system / toolchain header  */
#include "cv.h"       /* project header             */
```

- `#include <...>` searches the **system include paths** (the toolchain's own
  directories, plus anything added with `-I`). Used for standard/library headers.
- `#include "..."` searches **the current file's directory first**, then falls
  back to the same system paths. Used for your own project headers.

In this build, `cv.h` lives in `include/`, and `-Iinclude` in `CFLAGS` adds that
directory to the search path, so even the quote-form resolves through it. The
convention in the code is consistent and correct: toolchain types in angle
brackets, project interfaces in quotes. `<stdint.h>` itself comes from the
`arm-none-eabi` toolchain (for the firmware) or your host toolchain (for the
test) — one of the few headers `-ffreestanding` still guarantees.

---

## 3. Register-Level Peripheral Configuration

All hardware here is configured the **"inline register" way**: each register is a
`#define` that casts its absolute address to a `volatile` pointer and
dereferences it, e.g.

```c
#define SPI2_CR1 (*(volatile uint32_t *)0x40003800)
```

`volatile` is mandatory — it tells the compiler *"this memory can change or have
side effects outside the program's control; never cache it, never elide a read or
write, never reorder."* Without it, `while (!(SPI2_SR & SPI_SR_TXE)) {}` could be
"optimized" into reading the status once and looping forever.

### 3.1 The RCC clock-enable pattern (read this first)

On STM32, **every peripheral powers up with its clock gated off**, and a
clock-gated peripheral is electrically dead: its registers read as zero and ignore
writes. So the universal first step for *any* peripheral is to enable its clock in
the relevant **RCC** (Reset and Clock Control) register. The peripheral's bus
determines which register:

```c
RCC_AHB1ENR |= (1u << 1);    /* GPIOB on the AHB1 bus     (spi2.c)  */
RCC_APB1ENR |= (1u << 14);   /* SPI2  on the APB1 bus     (spi2.c)  */
RCC_AHB1ENR |= (1u << 3);    /* GPIOD on the AHB1 bus     (main.c)  */
```

- GPIO ports hang off **AHB1**, so their enables are in `RCC_AHB1ENR`
  (`0x40023830`). Bit 1 = GPIOB, bit 3 = GPIOD (bit *n* = port *n*, A=0, B=1…).
- SPI2 hangs off the slower **APB1** bus, so its enable is in `RCC_APB1ENR`
  (`0x40023840`), bit 14.

**Why order matters:** if you write a GPIO or SPI configuration register *before*
its clock is enabled, the write is silently dropped — the peripheral isn't
clocked, so it can't latch anything. The classic newcomer bug is "my GPIO config
does nothing"; nine times out of ten the clock-enable line is missing or after the
config. The `|=` (read-modify-write) is used rather than `=` so enabling one
peripheral's clock doesn't clobber others already enabled.

### 3.2 GPIO basic output — the LED (`main.c`)

```c
GPIOD_MODER &= ~(0x3u << (LED_PIN * 2));   /* clear the 2-bit field */
GPIOD_MODER |=  (0x1u << (LED_PIN * 2));    /* set it to 01 = output */
```

Each GPIO pin gets a **2-bit mode field** in `MODER`, so pin *n* occupies bits
`[2n+1 : 2n]`. `LED_PIN` is 12, so the field is bits `[25:24]`. The two-step
clear-then-set idiom is essential with multi-bit fields: you can't just OR in `01`
because if the field already held `11` you'd be left with `11`. You must first
mask the field to `00` (`&= ~(0x3 << ...)`) then OR in the desired value.

The 2-bit MODER encodings (memorize these — they recur everywhere):

| Value | Mode |
|---|---|
| `00` | Input |
| `01` | General-purpose **output** |
| `10` | **Alternate function** (peripheral-controlled) |
| `11` | Analog |

LED on PD12 → output (`01`). Then toggling is just `GPIOD_ODR ^= (1u <<
LED_PIN)` — XOR the **Output Data Register** bit each pass for the heartbeat.

### 3.3 GPIO alternate function — SPI pins (`spi2.c`)

A basic output pin is driven by your code through `ODR`. But SPI's clock and data
lines must be driven by the **SPI peripheral hardware**, toggling at MHz rates your
code can't match. That's what *alternate function* mode is for: it disconnects the
pin from `ODR` and hands control to an on-chip peripheral. Setting it up is two
registers:

```c
/* MODER: PB12 output(01), PB13/PB15 alternate function(10) */
GPIOB_MODER |= (0x1u << (CS_PIN*2)) | (0x2u << (SCK_PIN*2)) | (0x2u << (MOSI_PIN*2));
```

- **PB12 (CS) → output `01`.** Chip-select is driven by software as a plain GPIO
  (see §4.3), so it stays a basic output.
- **PB13 (SCK) and PB15 (MOSI) → alternate `10`.** Now the SPI2 peripheral owns
  them. But *which* peripheral? A pin can multiplex many functions (SPI, timer,
  UART…). That's selected separately:

```c
/* AFRH: AF5 (SPI2) on PB13 and PB15 */
GPIOB_AFRH |= (0x5u << ((SCK_PIN-8)*4)) | (0x5u << ((MOSI_PIN-8)*4));
```

The **alternate-function selection** uses a 4-bit field per pin, so 16 functions
(AF0–AF15) are selectable. 16 pins × 4 bits = 64 bits won't fit one 32-bit
register, so it's split: `AFRL` for pins 0–7, `AFRH` for pins 8–15. PB13/PB15 are
in the high half, hence `AFRH`, and the index is `(pin - 8) * 4`. The value `5`
(AF5) is the F407's mapping for SPI2's SCK/MOSI — **straight from the datasheet's
alternate-function table**; there's nothing derivable about it, you look it up. If
you set AF5 in `AFR` but forgot to set MODER to `10`, the pin would stay a GPIO and
SCK/MOSI would simply never move — a common silent failure.

The remaining GPIO setup tunes the electrical behavior of those three pins:

```c
GPIOB_OSPEEDR |= (0x2 << ...);  /* '10' = high speed   */
GPIOB_OTYPER  &= ~(...);        /* '0'  = push-pull    */
GPIOB_PUPDR   &= ~(...);        /* '00' = no pull      */
```

- **OSPEEDR = high speed (`10`).** Slew-rate control. SPI edges at ~2 MHz need
  fast enough output drivers that the signal is clean at the DAC; too slow a
  setting rounds the edges and the receiver mis-samples. (Faster than necessary
  costs EMI, hence it's a knob.)
- **OTYPER = push-pull (`0`).** The driver actively pulls both high and low. The
  alternative, open-drain (`1`), can only pull low and needs external pull-ups —
  appropriate for I²C, wrong for SPI.
- **PUPDR = no pull (`00`).** The pins are actively driven (by the peripheral, or
  by software for CS), so internal pull resistors are unnecessary.

```c
GPIOB_ODR |= (1u << CS_PIN);   /* idle CS high BEFORE enabling SPI */
```

CS is active-low on the MCP4922, so it must idle **high**. Setting it high here —
before the SPI peripheral is enabled — guarantees the DAC never sees a spurious
falling edge during bring-up.

### 3.4 SPI peripheral configuration — `SPI2_CR1` (`spi2.c`)

```c
SPI2_CR1 = (1u << 11) |  /* DFF  = 16-bit frame      */
           (1u << 9)  |  /* SSM  = software CS mgmt   */
           (1u << 8)  |  /* SSI  = internal NSS high  */
           (2u << 3)  |  /* BR   = fPCLK/8 (~2 MHz)   */
           (1u << 2);    /* MSTR = master             */
SPI2_CR1 |= (1u << 6);   /* SPE  = enable, set LAST   */
```

Here a plain `=` (not `|=`) is fine because the peripheral was just clocked and
`CR1` is in its reset (zero) state — we're writing the whole configuration at once.
Field by field:

| Bit(s) | Field | Value | Meaning & why |
|---|---|---|---|
| 11 | **DFF** | 1 | 16-bit data frame. The MCP4922 command word is exactly 16 bits, so one frame = one DAC update. With `DFF=0` you'd send 8 bits and need two transfers with CS subtleties; 16-bit is the clean match. This is also why `SPI2_DR` is accessed as a `uint16_t` (`SPI2_DR16`). |
| 9 | **SSM** | 1 | Software slave management. We don't use the hardware NSS pin for CS; we drive PB12 ourselves. SSM tells the peripheral to ignore the physical NSS and take the chip-select-internal level from SSI instead. |
| 8 | **SSI** | 1 | Internal NSS = high. With SSM=1, a master must hold its internal NSS high or it will think another master grabbed the bus and drop out of master mode (a MODF fault). SSI=1 keeps it happy. |
| 5:3 | **BR** | `010` (2) | Baud-rate divider = fPCLK / 8. APB1 runs at ~16 MHz out of reset, so SCK ≈ 2 MHz — comfortably under the MCP4922's 20 MHz limit, slow enough to be robust on a breadboard, fast enough that a 16-bit frame is ~8 µs. (See ADR-003.) |
| 2 | **MSTR** | 1 | Master mode. The STM32 generates the clock and initiates transfers; the DAC is the slave. |
| 1, 0 | **CPOL, CPHA** | 0, 0 | Left at reset = **SPI Mode 0**: clock idles low, data sampled on the rising edge. This is what the MCP4922 expects. (They aren't written explicitly; they're 0 because the whole register was assigned and those bits weren't set.) |
| — | **LSBFIRST** | 0 | Also implicitly 0 = MSB first, which the MCP4922 requires (its command word's top bit is the channel select). |
| 6 | **SPE** | 1 | SPI enable. **Set last, in a separate write,** because several config bits (DFF, CPOL/CPHA, BR) must not change while SPE=1. Configure fully, then flip the master switch. |

What would go wrong if these were off: wrong `BR` → exceed the DAC's max clock and
get corrupt data, or go needlessly slow. `CPOL/CPHA` wrong → data sampled on the
wrong edge → garbage. `SSI=0` with `SSM=1` → instant MODF, SPI silently disables
itself. `DFF=0` → you'd transmit 8-bit halves and the framing to the DAC breaks.
`MSTR=0` → the STM32 waits for an external clock that never comes.

---

## 4. SPI and the MCP4922 Protocol Layer

### 4.1 SPI at the signal level

SPI is a synchronous, full-duplex, master-driven bus with four logical signals:

- **SCK** — serial clock, generated by the master (STM32). Every bit is shifted on
  a clock edge; no clock, no data movement. This is why it's *synchronous* — there's
  no agreed baud rate to mismatch, the clock line *is* the timing.
- **MOSI** — Master Out, Slave In. The STM32 → MCP4922 data line. This carries the
  16-bit command word.
- **MISO** — Master In, Slave Out. Slave → master. **The MCP4922 has no data
  output** (it's a DAC — write-only), so MISO is unused here and the driver never
  reads received data. That's why `spi2.h` describes it as a "write-only" driver.
- **CS / NSS** — Chip Select, active low. Frames a transaction and selects which
  slave listens. Here it's PB12 driven by software (§4.3).

A transfer is a **shift exchange**: as the master clocks 16 bits out on MOSI, 16
bits simultaneously clock in on MISO. We ignore the inbound half.

### 4.2 The MCP4922 command word, bit by bit

The MCP4922 swallows one 16-bit word per write. From `mcp4922.c` /
`mcp4922.h`:

```c
cmd |= (channel & 0x1u) << 15;   /* bit 15 */
cmd |= 1u << 13;                 /* bit 13 */
cmd |= 1u << 12;                 /* bit 12 */
cmd |= value & 0x0FFFu;          /* bits 11:0 */
```

| Bit | Name | Set to | Meaning |
|---|---|---|---|
| 15 | **/A̅B̅** (channel) | 0 or 1 | 0 = DAC channel A (VOUTA), 1 = channel B. The mask `channel & 1` makes any nonzero argument select B safely. |
| 14 | **BUF** | 0 | Vref input **unbuffered**. Left 0 (the bit is simply never set). Buffering the reference raises input impedance; unbuffered is fine when Vref is a low-impedance rail. |
| 13 | **/G̅A̅** (gain) | 1 | Output gain = **1×** (so VOUT spans 0…Vref). `0` would mean 2× (0…2·Vref), which can't be reached if Vref = VDD. 1× is the right choice for a 3.3 V reference. |
| 12 | **/S̅H̅D̅N̅** | 1 | Output **active** (not shut down). `0` would tri-state the output. Must be 1 to get a voltage out. |
| 11:0 | **D11:D0** | 0…4095 | The 12-bit DAC code. Masked with `0x0FFF` so a stray high bit in `value` can't corrupt the control nibble. |

So the top nibble is always `0b0011` for channel A (`0x3xxx`) and `0b1011` for
channel B (`0xBxxx`) — exactly what the host tests assert (`0x3000`, `0xB000`,
etc.). This packing is a **pure function**: no hardware, fully deterministic,
which is why it's unit-tested on the host (§6).

### 4.3 CS timing — why CS wraps the whole 16-bit frame

```c
void spi2_write16(uint16_t data)
{
    GPIOB_ODR &= ~(1u << CS_PIN);     /* CS low: begin frame  */
    while (!(SPI2_SR & SPI_SR_TXE)) {} /* wait TX empty       */
    SPI2_DR16 = data;                  /* push 16 bits        */
    while (SPI2_SR & SPI_SR_BSY) {}    /* wait until done     */
    GPIOB_ODR |= (1u << CS_PIN);       /* CS high: latch       */
}
```

The MCP4922 **counts clock edges while CS is low** and acts on the data only at
the **rising edge of CS**. That has a hard requirement: CS must go low, *exactly*
16 clocks must occur, and CS must go high — all as one unit. If CS glitched high
mid-frame, the DAC would discard the partial word; if it stayed low across two
frames, the chip would see 32 clocks and misalign. So:

1. **CS low** *before* any clocking — opens the frame and tells the DAC "start
   counting."
2. The 16 clocks happen (the single `SPI2_DR16` write, DFF=1).
3. **CS high only after the bus is fully idle** — this rising edge is what
   transfers the input register to the DAC output (the value actually appears on
   VOUT here).

Because CS is software-driven, the **ordering of the two status waits is what makes
this correct** — see next.

### 4.4 The status-register polling pattern — and why it's correct

Two different flags in `SPI2_SR` are polled, and they are **not interchangeable**:

- **`TXE` (TX buffer empty, bit 1)** — checked *before* writing `SPI2_DR16`. It
  means the transmit *buffer* has room. We wait for it so we never overwrite a
  word that hasn't yet been handed to the shift register. For a single write to an
  idle SPI it's already set, but the pattern is correct for back-to-back sends.
- **`BSY` (busy, bit 7)** — checked *after* the write, before raising CS. It means
  the shift register is still clocking bits out on the wire. **This is the
  critical one for software CS.** `TXE` going high only means the *buffer* drained
  into the shifter — the last bits may still be in flight on MOSI/SCK. If we
  raised CS on `TXE` alone, CS could go high *before the 16th bit finished
  clocking*, truncating the frame to the DAC. Waiting for `BSY` to clear
  guarantees all 16 bits have physically left the pin, so the CS rising edge lands
  cleanly after bit 16.

This `TXE`-before / `BSY`-after sandwich is the textbook-correct pattern for a
**blocking, software-CS SPI write**, and the comment ordering in the code reflects
exactly that reasoning. (See ADR-002 for the CS-management decision.)

---

## 5. CV Output Math

### 5.1 The 1V/oct convention

Eurorack and most analog synths encode pitch as a control voltage where **one volt
equals one octave**, and an octave is 12 equal semitones, so **one semitone =
1/12 V ≈ 83.33 mV**. A module that puts out a precise 1V/oct CV will track pitch
across any compliant oscillator. Get the scaling wrong and notes play sharp/flat by
a fixed ratio — the most audible possible bug. This is why the conversion is
isolated and unit-tested rather than buried in the hardware path.

### 5.2 The conversion, line by line (`cv.c`)

```c
uint16_t note_to_dac(uint8_t midi_note)
{
    float semitones = (float)((int)midi_note - CV_REF_NOTE);  /* 1 */
    float voltage   = semitones / CV_SEMIS_OCT;               /* 2 */
    float count     = (voltage / CV_VREF) * CV_DAC_COUNTS;    /* 3 */

    if (count < 0.0f) return 0;                               /* 4 */
    count += 0.5f;                                            /* 5 */
    if (count > (float)CV_DAC_MAX) return CV_DAC_MAX;         /* 6 */
    return (uint16_t)count;                                   /* 7 */
}
```

1. **Semitones from the reference.** `CV_REF_NOTE = 60` (MIDI C4) maps to 0 V. The
   cast to `int` *before* subtracting matters: `midi_note` is `uint8_t`, so for
   notes below 60 the subtraction must happen in signed arithmetic or it would
   wrap to a huge positive number. Casting to `int` first keeps it signed.
2. **Semitones → volts.** Divide by 12 (`CV_SEMIS_OCT`). This is the 1V/oct law:
   12 semitones = 1 V. So note 72 (C5) → 12 semitones → 1.000 V.
3. **Volts → DAC count.** The DAC's full-scale (`CV_DAC_COUNTS = 4096`) spans
   `CV_VREF = 3.3 V`. So `count = (voltage / 3.3) * 4096`. The desired output
   voltage is expressed as a fraction of the reference, scaled to the code range.
4. **Clamp low.** Notes below C4 give negative voltage, which the DAC can't
   produce → clamp to 0. (The hardware can't go below ground here.)
5. **Round to nearest.** Adding 0.5 before the truncating cast turns floor into
   round-to-nearest, halving the worst-case quantization error.
6. **Clamp high.** The valid code range is 0…`CV_DAC_MAX = 4095`. Note that
   `CV_DAC_COUNTS` (4096) is the *span* used for scaling, while `CV_DAC_MAX`
   (4095) is the largest *code* — a 12-bit DAC has 4096 levels numbered 0…4095.
   Mixing these up by one is a classic off-by-one; the code keeps them distinct.
7. **Truncate to integer code.** After the +0.5, the cast yields the rounded
   12-bit value.

### 5.3 Where the constants come from (`cv.h`)

| Constant | Value | Derivation |
|---|---|---|
| `CV_SEMIS_OCT` | 12.0 | Definition of an octave (12 equal-tempered semitones), and by the 1V/oct law, also volts-per-octave's reciprocal basis. |
| `CV_REF_NOTE` | 60 | MIDI note number for C4, chosen as the 0 V anchor. Pure convention — move it and the whole keyboard shifts. |
| `CV_DAC_COUNTS` | 4096.0 | 2¹² — the number of levels in a 12-bit DAC, the scaling denominator. |
| `CV_DAC_MAX` | 4095 | 2¹² − 1 — the largest representable code, the clamp ceiling. |
| `CV_VREF` | 3.3 | The DAC reference voltage = the DISC1's 3.3 V rail. **The one calibration knob:** measure the actual rail and set this to the measured value to trim absolute pitch. |

**Worked example (C5, MIDI 72):** semitones = 72−60 = 12; voltage = 12/12 = 1.0 V;
count = (1.0/3.3)·4096 = 1241.2; +0.5 = 1241.7 → 1241. The test asserts exactly
1241, and 1241/4096·3.3 ≈ 0.9998 V out — matching the "≈1.000 V at C5" target in
`main.c`'s bench comment.

**The Vref caveat worth internalizing:** because pitch is `(voltage/Vref)·counts`,
the *output voltage* per semitone depends on Vref being accurate. If the real rail
is 3.28 V but `CV_VREF` says 3.30, every note is slightly flat by the same ratio.
That's deliberate: one constant, measured once, calibrates the whole instrument.

---

## 6. Code Quality and Architecture Observations

### 6.1 What's well-suited to bare metal here

- **Pure-vs-impure separation, enforced by the build.** `note_to_dac()` and
  `mcp4922_command()` are pure functions with zero hardware dependency, so they
  compile and run on the host and are covered by `make test` (21 assertions). The
  register I/O (`spi2_*`) is quarantined in its own translation unit and verified
  on the bench. This is the single best decision in the codebase: it makes the
  *most bug-prone, most audible* logic (pitch math, command framing) testable in
  CI on every push, without hardware in the loop.
- **Clean dependency layering via headers.** `main` → `dac` → {`mcp4922`,
  `spi2`}. `main.c` never includes `spi2.h`; the SPI driver is an implementation
  detail behind the DAC façade. Swapping SPI2 for SPI1 or DMA would touch one file.
- **Correct, defensible register discipline.** `volatile` everywhere; clock-enable
  before access; clear-then-set on multi-bit fields; SPE written last; CS idled
  high before enable; `BSY`-after-write before raising CS. These are exactly the
  details that bite newcomers, and they're all right.
- **Decisions are documented.** ADR-001..003 capture *why* SPI2, software CS, and
  /8 clock — so the rationale survives even when the code changes.
- **Self-documenting constants.** The CV math uses named `#define`s with a stated
  calibration knob, not bare numbers.

### 6.2 Things that will bite as the project grows

- **Register definitions are copy-pasted per file.** `RCC_AHB1ENR` and the GPIOB
  block are redefined in both `main.c` and `spi2.c`. Today they agree; the day
  they don't, you get a subtle divergence. As the ADC/audio path arrives, this
  duplication multiplies. **Recommendation:** introduce a single
  `stm32f407_regs.h` (or adopt CMSIS device headers) as the one source of truth
  for register addresses and bit positions, and include it everywhere. The
  existing `docs/reference/register-map.md` is effectively the spec for this
  header already.
- **Magic bit numbers at the call sites.** `(1u << 3)` for GPIOD clock, `(2u << 3)`
  for BR, `(1u << 6)` for SPE — correct, but the meaning lives only in the
  comment. Named macros (`RCC_AHB1ENR_GPIODEN`, `SPI_CR1_BR_DIV8`,
  `SPI_CR1_SPE`) would make miswrites compile-time visible and self-checking.
- **Busy-wait `delay()` is cycle-count-dependent.** `delay(800000)` at `-O0` is a
  guess that depends on optimization level and core clock. Fine for a blink; it
  will *silently change duration* the moment you raise `-O` or reconfigure the
  clock tree. A SysTick-based millisecond timer is the standard replacement and is
  a prerequisite for any real timing (sampling, gate lengths).
- **No fault handling.** Every exception vector points at `Default_Handler` (spin
  forever). The moment you add interrupts (ADC conversion-complete, timer),
  you'll want at least a HardFault handler that captures `CFSR`/`HFSR` (already
  listed in the register map doc) so a fault is debuggable rather than a silent
  hang.
- **Blocking SPI is fine now, not later.** `spi2_write16` busy-waits on `BSY`. For
  a single DAC write between long delays that's harmless. Under a real-time
  sampling loop it stalls the CPU for the whole frame; interrupt- or DMA-driven
  SPI will eventually be wanted.
- **No clock tree configuration.** The code runs on the default post-reset clock
  (HSI, ~16 MHz on APB1). That's why SCK is ~2 MHz at BR=/8. The audio/ADC path
  will likely need the PLL configured for a higher, *known* system clock — at
  which point every delay and the SPI baud rate shift, so this should be
  established deliberately before, not after, adding subsystems.

### 6.3 Before adding the ADC / audio-input path

In rough priority order:

1. **Centralize register definitions** (`stm32f407_regs.h` or CMSIS) — stops the
   duplication from spreading to the ADC block.
2. **Configure the clock tree explicitly** (PLL → known SYSCLK/HCLK/PCLK) — the
   ADC sample rate and SPI baud both derive from it; pin it down first.
3. **Add a SysTick time base** — replaces the fragile busy-wait `delay()` and
   gives you real timing for sampling.
4. **Add a real HardFault handler** — you're about to enable interrupts; make
   faults observable.
5. **Decide interrupt vs polling for the ADC** and, if interrupts, extend the
   vector table beyond the current 16 system entries to include the peripheral
   IRQs (the startup file's table will need the device-specific IRQ vectors
   appended).

None of these are present bugs — the current firmware is correct for what it does.
They are the load-bearing pieces the *next* subsystem will lean on.

---

## 7. Suggested Documentation Hooks

Short section titles and one-line summaries ready to drop into the MkDocs nav
(several map onto pages that already exist under `docs/` — noted where so):

| Suggested title | One-line summary | Maps to |
|---|---|---|
| **From Reset to main()** | What the startup file and vector table do in the gap between power-on and the first C instruction. | new (firmware) |
| **The Three-File Contract** | How the linker script, startup code, and Makefile interlock through six shared symbols. | new (firmware) |
| **Why a Linker Script** | What VMA vs LMA means and how `.data`/`.bss` get initialized on bare metal. | new (concepts) |
| **The Makefile, Flag by Flag** | Every `CFLAGS`/`LDFLAGS` option and why a freestanding ARM build needs it. | new (firmware) |
| **Headers Are Promises** | Declarations vs definitions, include guards, and the one-definition rule in this codebase. | new (concepts) |
| **The RCC Clock-Enable Rule** | Why every peripheral is dead until you ungate its clock — and why order matters. | `reference/register-map.md` |
| **GPIO Modes & Alternate Functions** | Output vs AF mode, the AF5 lookup, and how a pin gets handed to the SPI hardware. | `concepts/stm32-alternate-functions.md` |
| **SPI2 Configuration Decoded** | Every `SPI2_CR1` bit field and what breaks if it's wrong. | `concepts/spi-peripheral.md` |
| **The MCP4922 Command Word** | Bit-by-bit anatomy of the 16-bit DAC write word. | `reference/mcp4922-command-word.md` |
| **Software CS & the BSY Wait** | Why CS must wrap the whole frame and why we poll BSY before raising it. | `decisions/adr-002-cs-management.md` |
| **The Status-Polling Pattern** | TXE-before / BSY-after: the correct blocking SPI write. | `firmware/spi-driver.md` |
| **1V/oct in 12 Bits** | The note→voltage→count math and where every constant comes from. | `concepts/cv-math.md` |
| **The Vref Calibration Knob** | Why one measured constant trims the whole instrument's pitch. | `concepts/cv-math.md` |
| **Pure vs Impure: Why We Can Unit-Test** | The host-test boundary and how `make test` covers the audible logic. | `firmware/architecture.md` |
| **Growing Pains** | The register-duplication, magic-number, timing, and fault-handling debts to clear before the ADC path. | new (firmware) |

---

*This document reflects the firmware as of the SPI/DAC bring-up. When the clock
tree, SysTick, or ADC path land, revisit §6.2–6.3 — those sections are written to
become wrong (in a good way) as the project grows.*
