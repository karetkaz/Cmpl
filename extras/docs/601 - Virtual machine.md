# Virtual machine
The virtual machine is a very simple stack based one, with a minimal set of registers.

## Memory layout
The memory is organized in 4 segments:

- meta\[r--]:
	- identifiers and string constants (are emitted in the compilation phase).
	- typename and variable metadata (are emitted in the compilation phase).
	- enumeration values and initializer literals.

- code\[r-x]:
	- functions implementations.

- data\[rw-]:
	- static variables.

- heap\[rw-]:
	- heap memory
	- stack(s).

In the compilation phase the lower part of the memory is filled with persistent data which remains available for code execution,
while the upper part is filled with token structures and other temporary data, that will be overwritten on code execution.

## Registers
The virtual machine registers:
- ip: Instruction pointer
- sp: Stack pointer
- tp: Trace pointer (used only when debugging/profiling)
- bp: Base pointer constant (to detect stack overflow)
- ss: Stack size constant

## Instruction set

The instructions of the vm are grouped by type:
* `0x00 ... 0x0f` system instructions
* `0x10 ... 0x1f` stack manipulation
* `0x20 ... 0x2f` memory manipulation
* `0x30 ... 0x3f` unsigned 32-bit
* `0x40 ... 0x4f` unsigned 64-bit
* `0x50 ... 0x5f` signed 32-bit
* `0x60 ... 0x6f` signed 64-bit
* `0x70 ... 0x7f` float 32-bit
* `0x80 ... 0x8f` float 64-bit
* `0x90 ... 0x9f` packed128: float32x4
* `0xa0 ... 0xaf` packed128: float64x2, float32x4, int64x2, int32x4, int16x8, int8x16
* `0xb0 ... 0xbf` packed256: float64x4, float32x8, int64x4, int32x8, int16x16, int8x32
* `0xc0 ... 0xcf` u32u64ext: todo
* `0xd0 ... 0xdf` i32i64ext: todo
* `0xe0 ... 0xef` f32f64ext: todo
* `0xf0 ... 0xff` reserved

### Instruction matrix
|        | 0        | 1        | 2         | 3         | 4         | 5        | 6        | 7         | 8          | 9         | a        | b        | c         | d         | e          | f            |
|:------:|:---------|:---------|:----------|:----------|:----------|:---------|:---------|:----------|:-----------|:----------|:---------|:---------|:----------|:----------|:-----------|:-------------|
| 0:sys  | nop      | nfc      | call      | ret       | jmp       | jnz      | jz       | ~~fork~~  | ~~join~~   | ~~trap~~  | not      | inc      | mad       | -         | inc.sp     | ~~copy.mem~~ |
| 1:stk  | load.c32 | load.c64 | load.z32  | load.z64  | load.z128 | dup.x32  | dup.x64  | dup.x128  | set.x32    | set.x64   | set.x128 | mov.x32  | mov.x64   | mov.x128  | load.sp    | ~~load.ref~~ |
| 2:mem  | load.is8 | load.iu8 | load.is16 | load.iu16 | load.i32  | load.i64 | store.i8 | store.i16 | store.i32  | store.i64 | load.m32 | load.m64 | store.m32 | store.m64 | -          | -            |
| 3:u32  | cmt      | or       | xor       | mul       | div       | rem      | -        | and       | clt        | cgt       | shl      | shr      | sar       | ext.bit   | cvt.i64    | -            |
| 4:u64  | cmt      | or       | xor       | mul       | div       | rem      | -        | and       | clt        | cgt       | shl      | shr      | sar       | -         | -          | -            |
| 5:i32  | neg      | add      | sub       | mul       | div       | rem      | -        | ceq       | clt        | cgt       | cvt.bool | cvt.i64  | cvt.f32   | cvt.f64   | -          | -            |
| 6:i64  | neg      | add      | sub       | mul       | div       | rem      | -        | ceq       | clt        | cgt       | cvt.i32  | cvt.bool | cvt.f32   | cvt.f64   | -          | -            |
| 7:f32  | neg      | add      | sub       | mul       | div       | rem      | -        | ceq       | clt        | cgt       | cvt.i32  | cvt.i64  | cvt.bool  | cvt.f64   | -          | ~~load.f32~~ |
| 8:f64  | neg      | add      | sub       | mul       | div       | rem      | -        | ceq       | clt        | cgt       | cvt.i32  | cvt.i64  | cvt.f32   | cvt.bool  | -          | ~~load.f64~~ |
| 9:p128 | neg      | add      | sub       | mul       | div       | -        | -        | ceq       | min        | max       | dp3      | dp4      | dph       | swz       | load.i128  | load.m128    |
| a:p128 | neg      | add      | sub       | mul       | div       | -        | -        | ceq       | min        | max       | -        | -        | -         | -         | store.i128 | store.m128   |
| b:p256 | -        | -        | -         | -         | -         | -        | -        | -         | -          | -         | -        | -        | -         | -         | -          | -            |
| c:p512 | -        | -        | -         | -         | -         | -        | -        | -         | -          | -         | -        | -        | -         | -         | -          | -            |
| d:extU | -        | -        | -         | -         | -         | -        | -        | -         | -          | -         | -        | -        | -         | -         | -          | -            |
| e:extI | -        | -        | -         | -         | -         | -        | -        | -         | -          | -         | -        | -        | -         | -         | -          | -            |
| f:extF | -        | -        | -         | -         | -         | -        | -        | -         | -          | -         | -        | -        | -         | -         | -          | -            |
