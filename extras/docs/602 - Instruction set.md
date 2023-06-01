### Instruction `nop`

Performs no operation.

* Instruction code: `0x00`
* Instruction length: 1 byte
* Requires 0 operands: […
* Returns 0 values: […

```
// nothing
```

### Instruction `nfc`

Performs native ("C") function call.

* Instruction code: `0x01`
* Instruction length: 4 bytes
* Requires 0 operands: […, `result`, `params`
* Returns 0 values: […, `result`, `params`

```
callNative(imm.u24)
```

### Instruction `call`

Performs function call.

* Instruction code: `0x02`
* Instruction length: 1 byte
* Requires 1 operand: […, `result`, `params`, `FP`
* Returns 1 value: […, `result`, `params`, `IP`

```
FP = pop();   pop function pointer
push(IP);     push current instruction pointer (return address)
IP = FP;      jump to execute the function
```

### Instruction `ret`

Performs the return from a function call, a.k.a. an indirect jump.

* Instruction code: `0x03`
* Instruction length: 1 byte
* Requires 1 operand: […, `IP`
* Returns 0 value: […

```
IP = pop();     set instruction pointer to the previously saved one
```

### Instruction `jmp`

Performs an unconditional jump.

* Instruction code: `0x04`
* Instruction length: 4 bytes
* Requires 0 operands: […
* Returns 0 values: […

```
IP += imm.i24;     add the immediate value of the instruction to the instruction pointer
```

### Instruction `jnz`

Performs a conditional jump, if the top element on the stack is non-zero.

* Instruction code: `0x05`
* Instruction length: 4 bytes
* Requires 1 operand: […, `c`
* Returns 0 value: […

```
c = pop();
if (c != 0) {
	IP += imm.i24;
}
```

### Instruction `jz`

Performs a conditional jump, if the top element on the stack is zero.

* Instruction code: `0x06`
* Instruction length: 4 bytes
* Requires 1 operand: […, `c`
* Returns 0 value: […

```
c = pop();
if (c == 0) {
	IP += imm.i24;
}
```

### Instruction `task`

Try to delegate the execution to a different execution unit.

* Instruction code: `0x07`
* Instruction length: 4 bytes
* Requires 0 operands: […
* Returns 0 values: […

This instruction has 2 arguments:
* cl: code length 16 bit
* dl: data length 8 bit

The code to run in parallel starts after the current instruction, and ends after cl bytes.
The parallel running code may need to copy some local variables, which is denoted by dl.
In case the execution can not be delegated, it will be executed by the current execution unit.

```
if (ppu = acqireExecur()) {
	copyStack(ppu, imm.dl);
	ip += imm.cl;
}
```

### Instruction `sync`

Waits until all the delegated executions complete.

* Instruction code: `0x08`
* Instruction length: 2 bytes
* Requires 0 operands: […
* Returns 0 values: […

```
while (hasRunningWorkers) {
	park;
}
```

### Instruction `not.b32`

Performs logical not.

* Instruction code: `0x09`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `!a`

```
a = pop();
push(!a);
```

### Instruction `inc.i32`

Performs the increment of the top element of the stack.

* Instruction code: `0x0a`
* Instruction length: 4 bytes
* Requires 1 operand: […, `a`
* Returns 1 value: […, `a + imm`

```
a = pop();
push(a + imm.i24);
```

### Instruction `mad.u32`

Performs addition with immediate value multiplication.

* Instruction code: `0x0b`
* Instruction length: 4 bytes
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a + b * imm`

```
uint32 b = pop();
uint32 a = pop();
push(a + b * imm.u24)
```

### Instruction `inc.sp`

Performs stack memory management (allocation or free)

* Instruction code: `0x0e`
* Instruction length: 4 bytes

```
sp += imm.i24;
```

### Instruction `copy.mem`

[todo] temporary opcode, replace with native function call pointer.move(dst, src, count).


### Instruction `dup.x32`

Makes a copy of the nth stack element.

* Instruction code: `0x10`
* Instruction length: 2 bytes
* Requires 0 operands: […, `a`
* Returns 1 values: […, `a`, `a`
	* stack change example for: dup.x32(0)

```
a = sp(imm.u8);
push(a);
```

### Instruction `dup.x64`

Makes a copy of the nth stack element.

* Instruction code: `0x11`
* Instruction length: 2 bytes
* Requires 0 operands: […, `a1`, `a2`, `b`
* Returns 2 values: […, `a1`, `a2`, `b`, `a1`, `a2`
	* stack change example for: dup.x64(1)

```
a1 = sp(imm.u8 + 0);
a2 = sp(imm.u8 + 1);
push(a1, a2);
```

### Instruction `dup.x128`

Makes a copy of the nth stack element.

* Instruction code: `0x12`
* Instruction length: 2 bytes
* Requires 0 operands: […, `a1`, `a2`, `a3`, `a4`
* Returns 4 values: […, `a1`, `a2`, `a3`, `a4`, `a1`, `a2`, `a3`, `a4`
	* stack change example for: dup.x128(0)

```
a1 = sp(imm.u8 + 0);
a2 = sp(imm.u8 + 1);
a3 = sp(imm.u8 + 2);
a4 = sp(imm.u8 + 3);
push(a1, a2, a3, a4);
```

### Instruction `set.x32`

Update the value of the nth stack element.

* Instruction code: `0x13`
* Instruction length: 2 bytes
* Requires 1 operand: […, `b`, `a`
* Returns 0 value: […, `a`
	* stack change example for: set.x32(1)

```
if (imm.u8 < 1) {
	move1(sp + imm.u8, sp);
	drop(imm.u8);
	return;
}

a = pop();
sp[imm.u8 - 1] = a;
```

### Instruction `set.x64`

Update the value of the nth stack element.

* Instruction code: `0x14`
* Instruction length: 2 bytes
* Requires 1 operand: […, `c`, `b`, `a`, `a1`, `a2`
* Returns 0 value: […, `a1`, `a2`, `a`
	* stack change example for: set.x64(3)

```
if (imm.u8 < 2) {
	move2(sp + imm.u8, sp);
	drop(imm.u8);
	return;
}

a2 = pop();
a1 = pop();
sp[imm.u8 - 2] = a2;
sp[imm.u8 - 1] = a1;
```

### Instruction `set.x128`

Update the value of the nth stack element.

* Instruction code: `0x15`
* Instruction length: 2 bytes
* Requires 4 operands: […, `a`, `b`, `c`, `d`, `a1`, `a2`, `a3`, `a4`
* Returns 0 values: […, `a1`, `a2`, `a3`, `a4`
	* stack change example for: set.x128(4)

```
if (imm.u8 < 4) {
	move4(sp + imm.u8, sp);
	drop(imm.u8);
	return;
}

a4 = pop();
a3 = pop();
a2 = pop();
a1 = pop();
sp[imm.u8 - 4] = a4;
sp[imm.u8 - 3] = a3;
sp[imm.u8 - 2] = a2;
sp[imm.u8 - 1] = a1;
```

### Instruction `mov.x32`

Performs 32-bit data copy from the source to the destination position on the stack.

* Instruction code: `0x16`
* Instruction length: 3 bytes
* Requires 0 operands: […, `a`, `b`, `c`, `d`
* Returns 0 values: […, `c`, `b`, `c`, `d`
	* stack change example for: mov.x32(dst: 3, src: 1)

```
value = st[imm.src];
st[imm.dst] = value;
```

### Instruction `mov.x64`

Performs 64-bit data copy from the source to the destination positions on the stack.

* Instruction code: `0x17`
* Instruction length: 3 bytes
* Requires 0 operands: […, `a1`, `a2`, `b1`, `b2`, `c1`, `c2`
* Returns 0 values: […, `c1`, `c2`, `b1`, `b2`, `c1`, `c2`
	* stack change example for: mov.x64(dst: 4, src: 0)

```
value1 = sp(imm.src + 0);
value2 = sp(imm.src + 1);
sp[imm.dst + 0] = value1;
sp[imm.dst + 1] = value2;
```

### Instruction `mov.x128`

Performs 128-bit data copy from the source to the destination positions on the stack.

* Instruction code: `0x18`
* Instruction length: 3 bytes
* Requires 0 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 0 values: […, `a1`, `a2`, `a3`, `a4`, `a1`, `a2`, `a3`, `a4`
	* stack change example for: mov.x128(dst: 0, src: 4)

```
a1 = sp(imm.src + 0);
a2 = sp(imm.src + 1);
a2 = sp(imm.src + 2);
a2 = sp(imm.src + 3);
sp[imm.dst + 0] = a1;
sp[imm.dst + 1] = a2;
sp[imm.dst + 2] = a3;
sp[imm.dst + 3] = a4;
```

### Instruction `load.z32`

Push zero as a 32-bit value to the stack.

* Instruction code: `0x19`
* Instruction length: 1 byte
* Requires 0 operands: […
* Returns 1 values: […, 0

```
push(0);
```

### Instruction `load.z64`

Push zero as a 64-bit value to the stack.

* Instruction code: `0x1a`
* Instruction length: 1 byte
* Requires 0 operands: […
* Returns 2 values: […, 0, 0

```
push(0, 0);
```

### Instruction `load.z128`

Push zero as a 128-bit value to the stack.

* Instruction code: `0x1b`
* Instruction length: 1 byte
* Requires 0 operands: […
* Returns 4 values: […, 0, 0, 0, 0

```
push(0, 0, 0, 0);
```

### Instruction `load.c32`

Push constant as a 32-bit value to the stack.

* Instruction code: `0x1c`
* Instruction length: 5 bytes
* Requires 0 operands: […
* Returns 1 values: […, `a`

```
push(imm.u32);
```

### Instruction `load.c64`

Push constant as a 64-bit value to the stack.

* Instruction code: `0x1d`
* Instruction length: 9 bytes
* Requires 0 operands: […
* Returns 2 values: […, `a1`, `a2`

```
push(imm.u64);
```

### Instruction `load.sp`

Performs local variable(on stack) address loading.

* Instruction code: `0x1e`
* Instruction length: 4 bytes
* Requires 0 operands: […
* Returns 1 values: […, `ref`

```
push(sp + imm.i24);
```

### Instruction `load.ref`

[todo] temporary opcode, alias for load.c32 or load.c64 depending on pointer size.


### Instruction `load.is8`

Performs 8-bit indirect memory transfer from memory to stack with sign extend.

* Instruction code: `0x20`
* Instruction length: 1 byte
* Requires 1 operand: […, `ref`
* Returns 1 value: […, `val`

```
ref = pop();
val.i32 = mem(ref).i8;  // sign extended
push(val);
```

### Instruction `load.iu8`

Performs 8-bit indirect memory transfer from memory to stack with zero extend.

* Instruction code: `0x21`
* Instruction length: 1 byte
* Requires 1 operand: […, `ref`
* Returns 1 value: […, `val`

```
ref = pop();
val.i32 = mem(ref).u8;  // zero extended
push(val);
```

### Instruction `load.is16`

Performs 16-bit indirect memory transfer from memory to stack with sign extend.

* Instruction code: `0x22`
* Instruction length: 1 byte
* Requires 1 operand: […, `ref`
* Returns 1 value: […, `val`

```
ref = pop();
val = mem(ref).i16;  // sign extended
push(val);
```

### Instruction `load.iu16`

Performs 16-bit indirect memory transfer from memory to stack with zero extend.

* Instruction code: `0x23`
* Instruction length: 1 byte
* Requires 1 operand: […, `ref`
* Returns 1 value: […, `val`

```
ref = pop();
val = mem(ref).u16;  // zero extended
push(val);
```

### Instruction `load.i32`

Performs 32-bit indirect memory transfer from memory to stack.

* Instruction code: `0x24`
* Instruction length: 1 byte
* Requires 1 operand: […, `ref`
* Returns 1 value: […, `val`

```
ref = pop();
val = mem(ref).u32;
push(val);
```

### Instruction `load.i64`

Performs 64-bit indirect memory transfer from memory to stack.

* Instruction code: `0x25`
* Instruction length: 1 byte
* Requires 1 operand: […, `ref`
* Returns 2 value: […, `a1`, `a2`

```
ref = pop();
a1, a2 = mem(ref).u64;
push(a1, a2);
```

### Instruction `load.i128`

Performs 128-bit indirect memory transfer from memory to stack.

* Instruction code: `0x26`
* Instruction length: 1 byte
* Requires 1 operand: […, `ref`
* Returns 4 value: […, `a1`, `a2`, `a3`, `a4`

```
ref = pop();
a1, a2, a3, a4 = mem(ref).u128;
push(a1, a2, a3, a4);
```

### Instruction `store.i8`

Performs 8-bit indirect memory transfer from stack to memory.

* Instruction code: `0x27`
* Instruction length: 1 byte
* Requires 2 operands: […, `val`, `ref`
* Returns 0 values: […

```
ref = pop();
val = pop();
mem[ref].u8 = val.u8;
```

### Instruction `store.i16`

Performs 16-bit indirect memory transfer from stack to memory.

* Instruction code: `0x28`
* Instruction length: 1 byte
* Requires 2 operands: […, `val`, `ref`
* Returns 0 values: […

```
ref = pop();
val = pop();
mem[ref].u16 = val.u16;
```

### Instruction `store.i32`

Performs 32-bit indirect memory transfer from stack to memory.

* Instruction code: `0x29`
* Instruction length: 1 byte
* Requires 2 operands: […, `val`, `ref`
* Returns 0 values: […

```
ref = pop();
val = pop();
mem[ref] = val;
```

### Instruction `store.i64`

Performs 64-bit indirect memory transfer from stack to memory.

* Instruction code: `0x2a`
* Instruction length: 1 byte
* Requires 3 operands: […, `a1`, `a2`, `ref`
* Returns 0 values: […

```
ref = pop();
a2 = pop();
a1 = pop();
mem[ref] = a1, a2;
```

### Instruction `store.i128`

Performs 128-bit indirect memory transfer from stack to memory.

* Instruction code: `0x2b`
* Instruction length: 1 byte
* Requires 5 operands: […, `a1`, `a2`, `a3`, `a4`, `ref`
* Returns 0 values: […

```
ref = pop();
a4 = pop();
a3 = pop();
a2 = pop();
a1 = pop();
mem[ref] = a1, a2, a3, a4;
```

### Instruction `load.m32`

Loads 32-bit value from the memory to the stack.

* Instruction code: `0x2c`
* Instruction length: 4 bytes
* Requires 0 operands: […
* Returns 1 values: […, `a`

compressed version for:
* `load.ref ref`
* `load.i32`

```
a = mem(imm.u24);
push(a);
```

### Instruction `load.m64`

Loads 64-bit value from the memory to the stack.

* Instruction code: `0x2d`
* Instruction length: 4 bytes
* Requires 0 operands: […
* Returns 2 values: […, `a1`, `a2`

compressed version for:
* `load.ref ref`
* `load.i64`

```
a1, a2 = mem2(imm.u24);
push(a1, a2);
```

### Instruction `store.m32`

Transfers 32-bit value from the stack to the memory.

* Instruction code: `0x2e`
* Instruction length: 4 bytes
* Requires 1 operand: […, `a`
* Returns 0 value: […

compressed version for:
* `load.ref ref`
* `store.i32`

```
a = pop();
mem[imm.u24] = a;
```

### Instruction `store.m64`

Transfers 64-bit value from the stack to the memory.

* Instruction code: `0x2f`
* Instruction length: 4 bytes
* Requires 2 operands: […, `a1`, `a2`
* Returns 0 values: […

compressed version for:
* `load.ref ref`
* `store.i64`

```
a2 = pop();
a1 = pop();
mem[imm.u24] = a1, a2;
```


### Instruction `cmt.b32`

Performs inversion of each bit in the top 32-bit integer operand on the stack.

* Instruction code: `0x30`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `~a`

```
uint32 a = pop()
push(~a)
```

### Instruction `and.b32`

Performs bitwise and between the two top 32-bit integer operands on the stack.

* Instruction code: `0x31`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a & b`

```
uint32 b = pop()
uint32 a = pop()
push(a & b)
```

### Instruction `or.b32`

Performs bitwise or between the two top 32-bit integer operands on the stack.

* Instruction code: `0x32`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a | b`

```
uint32 b = pop()
uint32 a = pop()
push(a | b)
```

### Instruction `mul.u32`

Performs unsigned multiplication of the two top 32-bit integer operands on the stack.

* Instruction code: `0x33`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a * b`

```
uint32 b = pop()
uint32 a = pop()
push(a * b)
```

### Instruction `div.u32`

Performs unsigned division of the two top 32-bit integer operands on the stack.

* Instruction code: `0x34`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a / b`

```
uint32 b = pop()
uint32 a = pop()
push(a / b)
```

### Instruction `mod.u32`

Performs unsigned remainder of the two top 32-bit integer operands on the stack.

* Instruction code: `0x35`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a % b`

```
uint32 b = pop()
uint32 a = pop()
push(a % b)
```

### Instruction `xor.b32`

Performs bitwise exclusive or between the two top 32-bit integer operands on the stack.

* Instruction code: `0x36`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a ^ b`

```
uint32 b = pop()
uint32 a = pop()
push(a ^ b)
```

### Instruction `clt.u32`

Performs unsigned less than comparison between the two top 32-bit integer operands on the stack.

* Instruction code: `0x38`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a < b`

```
uint32 b = pop()
uint32 a = pop()
push(a < b)
```

### Instruction `cgt.u32`

Performs unsigned greater than comparison between the two top 32-bit integer operands on the stack.

* Instruction code: `0x39`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a > b`

```
uint32 b = pop()
uint32 a = pop()
push(a > b)
```

### Instruction `shl.b32`

Performs bitwise left shift of the 32-bit integer operand.

* Instruction code: `0x3a`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a << b`

```
uint32 b = pop()
uint32 a = pop()
push(a << b)
```

### Instruction `shr.b32`

Performs bitwise right shift of the 32-bit unsigned integer operand.

* Instruction code: `0x3b`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a >> b`

```
uinte32 b = pop()
uinte32 a = pop()
push(a >> b)
```

### Instruction `sar.b32`

Performs arithmetic right shift of the 32-bit signed integer operand.

* Instruction code: `0x3c`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a >>> b`

```
uint32 b = pop()
int32 a = pop()
push(a >>> b)
```

### Instruction `u32.2i64`

Performs 32-bit to 64-bit integer conversion using zero extend.

* Instruction code: `0x3e`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 2 value: […, `0`, `a`

```
uint32 a = pop()
push(uint64(a))
```

### Instruction `b32.ext`

Performs bitwise and, shl, shr with immediate value.

* Instruction code: `0x3f`
* Instruction length: 2 bytes
* Requires 1 operand: […, `a`
* Returns 1 value: […, `a op imm`

```
a = pop();
b = imm.u8 & 0x3f
op = imm.u8 >> 6;
swotch(op) {
	case 0:
		push(a & ((1 << b) - 1))
		break
	case 1:
		push(a << b)
		break
	case 2:
		push(int32(a) >> b)
		break
	case 3:
		push(uint32(a) >> b)
		break
}
```


### Instruction `cmt.b64`

Performs inversion of each bit in the top 64-bit integer operand on the stack.

* Instruction code: `0x40`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 2 values: […, `~a`, `~a`

```
uint64 a = pop2()
push(~a)
```

### Instruction `and.b64`

Performs bitwise and between the two top 64-bit integer operands on the stack.

* Instruction code: `0x41`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a & b`, `a & b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a & b)
```

### Instruction `or.b64`

Performs bitwise or between the two top 64-bit integer operands on the stack.

* Instruction code: `0x42`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a | b`, `a | b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a | b)
```

### Instruction `mul.u64`

Performs unsigned multiplication on the two top 64-bit integer operands on the stack.

* Instruction code: `0x43`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a * b`, `a * b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a * b)
```

### Instruction `div.u64`

Performs unsigned division on the two top 64-bit integer operands on the stack.

* Instruction code: `0x44`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a / b`, `a / b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a / b)
```

### Instruction `mod.u64`

Performs unsigned remainder between the two top 64-bit integer operands on the stack.

* Instruction code: `0x45`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a % b`, `a % b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a % b)
```

### Instruction `xor.b64`

Performs bitwise exclusive or between the two top 64-bit integer operands on the stack.

* Instruction code: `0x46`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a ^ b`, `a ^ b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a ^ b)
```

### Instruction `clt.u64`

Performs unsigned less than comparison between the two top 64-bit integer operands on the stack.

* Instruction code: `0x48`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a < b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a < b)
```

### Instruction `cgt.u64`

Performs unsigned greater than comparison between the two top 64-bit integer operands on the stack.

* Instruction code: `0x49`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a < b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a > b)
```

### Instruction `shl.b64`

Performs bitwise left shift of the 64-bit integer operand.

* Instruction code: `0x4a`
* Instruction length: 1 byte
* Requires 3 operands: […, `a`, `a`, `b`
* Returns 2 values: […, `a << b`, `a << b`

```
uint32 b = pop()
uint64 a = pop2()
push(a << b)
```

### Instruction `shr.b64`

Performs bitwise right shift of the 64-bit unsigned integer operand.

* Instruction code: `0x4b`
* Instruction length: 1 byte
* Requires 3 operands: […, `a`, `a`, `b`
* Returns 2 values: […, `a >> b`, `a >> b`

```
uinte32 b = pop()
uinte64 a = pop2()
push(a >> b)
```

### Instruction `sar.b64`

Performs arithmetic right shift of the 64-bit signed integer operand.

* Instruction code: `0x4c`
* Instruction length: 1 byte
* Requires 3 operands: […, `a`, `a`, `b`
* Returns 2 values: […, `a >>> b`, `a >>> b`

```
uint32 b = pop()
int64 a = pop2()
push(a >>> b)
```


### Instruction `neg.i32`

Performs sign change on the topmost 32-bit integer operand.

* Instruction code: `0x50`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `-a`

```
int32 a = pop()
push(-a)
```

### Instruction `add.i32`

Performs addition of the two top 32-bit integer operands on the stack.

* Instruction code: `0x51`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a + b`

```
int32 b = pop()
int32 a = pop()
push(a + b)
```

### Instruction `sub.i32`

Performs subtraction of the two top 32-bit integer operands on the stack.

* Instruction code: `0x52`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a - b`

```
int32 b = pop()
int32 a = pop()
push(a - b)
```

### Instruction `mul.i32`

Performs signed multiplication of the two top 32-bit integer operands on the stack.

* Instruction code: `0x53`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a * b`

```
int32 b = pop()
int32 a = pop()
push(a * b)
```

### Instruction `div.i32`

Performs signed division between the two top 32-bit integer operands on the stack.

* Instruction code: `0x54`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a / b`

```
int32 b = pop()
int32 a = pop()
push(a / b)
```

### Instruction `mod.i32`

Performs signed remainder of the two top 32-bit integer operands on the stack.

* Instruction code: `0x55`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a % b`

```
int32 b = pop()
int32 a = pop()
push(a % b)
```

### Instruction `ceq.i32`

Performs equality comparison of the two top 32-bit integer operands on the stack.

* Instruction code: `0x57`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a == b`

```
int32 b = pop()
int32 a = pop()
push(a == b)
```

### Instruction `clt.i32`

Performs signed less than comparison of the two top 32-bit integer operands on the stack.

* Instruction code: `0x58`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a < b`

```
int32 b = pop()
int32 a = pop()
push(a < b)
```

### Instruction `cgt.i32`

Performs signed greater than comparison of the two top 32-bit integer operands on the stack.

* Instruction code: `0x59`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a > b`

```
int32 b = pop()
int32 a = pop()
push(a > b)
```

### Instruction `i32.2bool`

Performs 32-bit integer to boolean conversion.

* Instruction code: `0x5a`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `a != 0`

```
int32 a = pop()
push(a != 0)
```

### Instruction `i32.2i64`

Performs 32-bit to 64-bit integer conversion using sign extend.

* Instruction code: `0x5b`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 2 value: […, `a`, `a`

```
int32 a = pop()
push(int64(a))
```

### Instruction `i32.2f32`

Performs 32-bit integer to 32-bit floating point conversion.

* Instruction code: `0x5c`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `a`

```
int32 a = pop()
push(float32(a))
```

### Instruction `i32.2f64`

Performs 32-bit integer to 64-bit floating point conversion.

* Instruction code: `0x5d`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 2 value: […, `a`, `a`

```
int32 a = pop()
push(float64(a))
```


### Instruction `neg.i64`

Performs sign change on the topmost 64-bit integer operand.

* Instruction code: `0x60`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 2 values: […, `-a`, `-a`

```
int64 a = pop2()
push(-a)
```

### Instruction `add.i64`

Performs addition of the two top 64-bit integer operands on the stack.

* Instruction code: `0x61`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a + b`, `a + b`

```
int64 b = pop2()
int64 a = pop2()
push(a + b)
```

### Instruction `sub.i64`

Performs subtraction of the two top 64-bit integer operands on the stack.

* Instruction code: `0x62`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a - b`, `a - b`

```
int64 b = pop2()
int64 a = pop2()
push(a - b)
```

### Instruction `mul.i64`

Performs signed multiplication of the two top 64-bit integer operands on the stack.

* Instruction code: `0x63`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a * b`, `a * b`

```
int64 b = pop2()
int64 a = pop2()
push(a * b)
```

### Instruction `div.i64`

Performs signed division between the two top 64-bit integer operands on the stack.

* Instruction code: `0x64`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a / b`, `a / b`

```
int64 b = pop2()
int64 a = pop2()
push(a / b)
```

### Instruction `mod.i64`

Performs signed remainder of the two top 64-bit integer operands on the stack.

* Instruction code: `0x65`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a % b`, `a % b`

```
int64 b = pop2()
int64 a = pop2()
push(a % b)
```

### Instruction `ceq.i64`

Performs equality comparison between the two top 64-bit integer operands on the stack.

* Instruction code: `0x67`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a == b`

```
int64 b = pop2()
int64 a = pop2()
push(a == b)
```

### Instruction `clt.i64`

Performs signed less than comparison of the two top 64-bit integer operands on the stack.

* Instruction code: `0x68`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a < b`

```
int64 b = pop2()
int64 a = pop2()
push(a < b)
```

### Instruction `cgt.i64`

Performs signed greater than comparison of the two top 64-bit integer operands on the stack.

* Instruction code: `0x69`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a > b`

```
int64 b = pop2()
int64 a = pop2()
push(a > b)
```

### Instruction `i64.2i32`

Performs 64-bit to 32-bit integer conversion keeping only the low bits.

* Instruction code: `0x6a`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 1 values: […, `a`

```
int64 a = pop2()
push(int32(a))
```

### Instruction `i64.2bool`

Performs 64-bit integer to boolean conversion.

* Instruction code: `0x6b`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 1 values: […, `a != 0`

```
int64 a = pop2()
push(a != 0)
```

### Instruction `i64.2f32`

Performs 64-bit integer to 32-bit floating point conversion.

* Instruction code: `0x6c`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 1 values: […, `a`

```
int64 a = pop2()
push(float32(a))
```

### Instruction `i64.2f64`

Performs 64-bit integer to 64-bit floating point conversion.

* Instruction code: `0x6d`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 2 values: […, `a`, `a`

```
int64 a = pop2()
push(float64(a))
```


### Instruction `neg.f32`

Performs sign change on the topmost 32-bit floating point operand.

* Instruction code: `0x70`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `-a`

```
float32 a = pop()
push(-a)
```

### Instruction `add.f32`

Performs addition of the two top 32-bit floating point operands on the stack.

* Instruction code: `0x71`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a + b`

```
float32 b = pop()
float32 a = pop()
push(a + b)
```

### Instruction `sub.f32`

Performs subtraction of the two top 32-bit floating point operands on the stack.

* Instruction code: `0x72`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a - b`

```
float32 b = pop()
float32 a = pop()
push(a - b)
```

### Instruction `mul.f32`

Performs multiplication of the two top 32-bit floating point operands on the stack.

* Instruction code: `0x73`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a * b`

```
float32 b = pop()
float32 a = pop()
push(a * b)
```

### Instruction `div.f32`

Performs division between the two top 32-bit floating point operands on the stack.

* Instruction code: `0x74`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a / b`

```
float32 b = pop()
float32 a = pop()
push(a / b)
```

### Instruction `mod.f32`

Performs remainder of the two top 32-bit floating point operands on the stack.

* Instruction code: `0x75`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a % b`

```
float32 b = pop()
float32 a = pop()
push(a % b)
```

### Instruction `ceq.f32`

Performs equality comparison of the two top 32-bit floating point operands on the stack.

* Instruction code: `0x77`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a == b`

```
float32 b = pop()
float32 a = pop()
push(a == b)
```

### Instruction `clt.f32`

Performs less than comparison of the two top 32-bit floating point operands on the stack.

* Instruction code: `0x78`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a < b`

```
float32 b = pop()
float32 a = pop()
push(a < b)
```

### Instruction `cgt.f32`

Performs greater than comparison of the two top 32-bit floating point operands on the stack.

* Instruction code: `0x79`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a > b`

```
float32 b = pop()
float32 a = pop()
push(a > b)
```

### Instruction `f32.2i32`

Performs 32-bit floating point to 32-bit integer conversion.

* Instruction code: `0x7a`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `a`

```
float32 a = pop()
push(int32(a))
```

### Instruction `f32.2i64`

Performs 32-bit floating point to 64-bit integer conversion.

* Instruction code: `0x7b`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 2 value: […, `a`, `a`

```
float32 a = pop()
push(int64(a))
```

### Instruction `f32.2bool`

Performs 32-bit floating point to boolean conversion.

* Instruction code: `0x7c`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `a != 0`

```
float32 a = pop()
push(a != 0)
```

### Instruction `f32.2f64`

Performs 32-bit to 64-bit floating point conversion.

* Instruction code: `0x7d`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 2 value: […, `a`, `a`

```
float32 a = pop()
push(float64(a))
```

### Instruction `load.f32`

[todo] temporary opcode, alias for load.c32.


### Instruction `neg.f64`

Performs sign change on the topmost 64-bit floating point operand.

* Instruction code: `0x80`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 2 values: […, `-a`, `-a`

```
float64 a = pop2()
push(-a)
```

### Instruction `add.f64`

Performs addition of the two top 64-bit floating point operands on the stack.

* Instruction code: `0x81`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a + b`, `a + b`

```
float64 b = pop2()
float64 a = pop2()
push(a + b)
```

### Instruction `sub.f64`

Performs subtraction of the two top 64-bit floating point operands on the stack.

* Instruction code: `0x82`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a - b`, `a - b`

```
float64 b = pop2()
float64 a = pop2()
push(a - b)
```

### Instruction `mul.f64`

Performs multiplication of the two top 64-bit floating point operands on the stack.

* Instruction code: `0x83`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a * b`, `a * b`

```
float64 b = pop2()
float64 a = pop2()
push(a * b)
```

### Instruction `div.f64`

Performs division between the two top 64-bit floating point operands on the stack.

* Instruction code: `0x84`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a / b`, `a / b`

```
float64 b = pop2()
float64 a = pop2()
push(a / b)
```

### Instruction `mod.f64`

Performs remainder of the two top 64-bit floating point operands on the stack.

* Instruction code: `0x85`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a % b`, `a % b`

```
float64 b = pop2()
float64 a = pop2()
push(a % b)
```

### Instruction `ceq.f64`

Performs equality comparison of the two top 64-bit floating point operands on the stack.

* Instruction code: `0x87`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a == b`

```
float64 b = pop2()
float64 a = pop2()
push(a == b)
```

### Instruction `clt.f64`

Performs less than comparison of the two top 64-bit floating point operands on the stack.

* Instruction code: `0x88`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a < b`

```
float64 b = pop2()
float64 a = pop2()
push(a < b)
```

### Instruction `cgt.f64`

Performs greater than comparison of the two top 64-bit floating point operands on the stack.

* Instruction code: `0x89`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a > b`

```
float64 b = pop2()
float64 a = pop2()
push(a > b)
```

### Instruction `f64.2i32`

Performs 64-bit floating point to 32-bit integer conversion.

* Instruction code: `0x8a`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 1 values: […, `a`

```
float64 a = pop2()
push(int32(a))
```

### Instruction `f64.2i64`

Performs 64-bit floating point to 64-bit integer conversion.

* Instruction code: `0x8b`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 2 values: […, `a`, `a`

```
float64 a = pop2()
push(int64(a))
```

### Instruction `f64.2f32`

Performs 64-bit to 32-bit floating point conversion.

* Instruction code: `0x8c`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 1 values: […, `a`

```
float64 a = pop2()
push(float32(a))
```

### Instruction `f64.2bool`

Performs 64-bit floating point to boolean conversion.

* Instruction code: `0x8d`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 1 values: […, `a != 0`

```
float64 a = pop2()
push(a != 0)
```

### Instruction `load.f64`

[todo] temporary opcode, alias for load.c64.


### Instruction `neg.v4f`

Performs per component sign change on the topmost 32-bit floating point vector operand.

* Instruction code: `0x90`
* Instruction length: 1 byte
* Requires 4 operands: […, `a1`, `a2`, `a3`, `a4`
* Returns 4 values: […, `-a1`, `-a2`, `-a3`, `-a4`

```
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(-a1, -a2, -a3, -a4)
```

### Instruction `add.v4f`

Performs per component addition of the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x91`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 4 values: […, `a1 + b1`, `a2 + b2`, `a3 + b3`, `a4 + b4`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 + b1, a2 + b2, a3 + b3, a4 + b4)
```

### Instruction `sub.v4f`

Performs per component subtraction of the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x92`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 4 values: […, `a1 - b1`, `a2 - b2`, `a3 - b3`, `a4 - b4`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 - b1, a2 - b2, a3 - b3, a4 - b4)
```

### Instruction `mul.v4f`

Performs per component multiplication of the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x93`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 4 values: […, `a1 * b1`, `a2 * b2`, `a3 * b3`, `a4 * b4`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 * b1, a2 * b2, a3 * b3, a4 * b4)
```

### Instruction `div.v4f`

Performs per component division of the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x94`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 4 values: […, `a1 / b1`, `a2 / b2`, `a3 / b3`, `a4 / b4`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 / b1, a2 / b2, a3 / b3, a4 / b4)
```

### Instruction `ceq.v4f`

Performs per component equality comparison of the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x97`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 1 values: […, `a1 == b1 && a2 == b2 && a3 == b3 && a4 == b4`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 == b1 && a2 == b2 && a3 == b3 && a4 == b4)
```

### Instruction `min.v4f`

Performs per component minimum calculation on the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x98`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 4 values: […, `min(a1, b1)`, `min(a2, b2)`, `min(a3, b3)`, `min(a4, b4)`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(min(a1, b1), min(a2, b2), min(a3, b3), min(a4, b4))
```

### Instruction `max.v4f`

Performs per component maximum calculation on the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x99`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 4 values: […, `max(a1, b1)`, `max(a2, b2)`, `max(a3, b3)`, `max(a4, b4)`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(max(a1, b1), max(a2, b2), max(a3, b3), max(a4, b4))
```

### Instruction `dp3.v4f`

Performs 3 element dot product calculation on the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x9a`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 1 values: […, `a1 * b1 + a2 * b2 + a3 * b3`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 * b1 + a2 * b2 + a3 * b3)
```

### Instruction `dp4.v4f`

Performs 4 element dot product calculation on the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x9b`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 1 values: […, `a1 * b1 + a2 * b2 + a3 * b3 + a4 * b4`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 * b1 + a2 * b2 + a3 * b3 + a4 * b4)
```

### Instruction `dph.v4f`

Performs homogeneous dot product calculation on the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x9c`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 1 values: […, `a1 * b1 + a2 * b2 + a3 * b3 + a4`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 * b1 + a2 * b2 + a3 * b3 + a4)
```

### Instruction `swz.p4x`

Performs a component wise shuffling of the elements.

* Instruction code: `0x9d`
* Instruction length: 2 bytes
* Requires 4 operands: […, `a`, `b`, `c`, `d`
* Returns 4 values: […, `a`, `b`, `a`, `b`
	* stack change example for: swz.p4x 0x44

```
float32 d = pop()
float32 c = pop()
float32 b = pop()
float32 a = pop()
shuffle(&a, &b, &c, &d, imm.u8)
push(a, b, c, d)
```

### Instruction `load.m128`

Loads 128-bit value from the memory to the stack.

* Instruction code: `0x9e`
* Instruction length: 4 bytes
* Requires 0 operands: […
* Returns 4 values: […, `a1`, `a2`, `a3`, `a4`

compressed version for:
* load.ref var
* load.i128

```
a1, a2, a3, a4 = mem4(imm.u24);
push(a1, a2, a3, a4);
```

### Instruction `store.m128`

Transfers 128-bit value from the stack to the memory.

* Instruction code: `0x9f`
* Instruction length: 4 bytes
* Requires 4 operands: […, `a1`, `a2`, `a3`, `a4`
* Returns 0 values: […

compressed version for:
* `load.ref ref`
* `store.i128`

```
a4 = pop();
a3 = pop();
a2 = pop();
a1 = pop();
mem[imm.u24] = a1, a2, a3, a4;
```


### Instruction `neg.v2d`

Performs per component sign change on the topmost 64-bit floating point vector operand.

* Instruction code: `0xa0`
* Instruction length: 1 byte
* Requires 4 operands: […, `a1`, `a1`, `a2`, `a2`
* Returns 4 values: […, `-a1`, `-a1`, `-a2`, `-a2`

```
float64 a2 = pop2()
float64 a1 = pop2()
push(-a1, -a2)
```

### Instruction `add.v2d`

Performs per component addition of the two top 64-bit floating point vector operands on the stack.

* Instruction code: `0xa1`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a1`, `a2`, `a2`, `b1`, `b1`, `b2`, `b2`
* Returns 4 values: […, `a1 + b1`, `a1 + b1`, `a2 + b2`, `a2 + b2`

```
float64 b2 = pop2()
float64 b1 = pop2()
float64 a2 = pop2()
float64 a1 = pop2()
push(a1 + b1, a2 + b2)
```

### Instruction `sub.v2d`

Performs per component subtraction of the two top 64-bit floating point vector operands on the stack.

* Instruction code: `0xa2`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a1`, `a2`, `a2`, `b1`, `b1`, `b2`, `b2`
* Returns 4 values: […, `a1 - b1`, `a1 - b1`, `a2 - b2`, `a2 - b2`

```
float64 b2 = pop2()
float64 b1 = pop2()
float64 a2 = pop2()
float64 a1 = pop2()
push(a1 - b1, a2 - b2)
```

### Instruction `mul.v2d`

Performs per component multiplication of the two top 64-bit floating point vector operands on the stack.

* Instruction code: `0xa3`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a1`, `a2`, `a2`, `b1`, `b1`, `b2`, `b2`
* Returns 4 values: […, `a1 * b1`, `a1 * b1`, `a2 * b2`, `a2 * b2`

```
float64 b2 = pop2()
float64 b1 = pop2()
float64 a2 = pop2()
float64 a1 = pop2()
push(a1 * b1, a2 * b2)
```

### Instruction `div.v2d`

Performs per component division of the two top 64-bit floating point vector operands on the stack.

* Instruction code: `0xa4`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a1`, `a2`, `a2`, `b1`, `b1`, `b2`, `b2`
* Returns 4 values: […, `a1 / b1`, `a1 / b1`, `a2 / b2`, `a2 / b2`

```
float64 b2 = pop2()
float64 b1 = pop2()
float64 a2 = pop2()
float64 a1 = pop2()
push(a1 / b1, a2 / b2)
```

### Instruction `ceq.v2d`

Performs per component equality comparison of the two top 64-bit floating point vector operands on the stack.

* Instruction code: `0xa7`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a1`, `a2`, `a2`, `b1`, `b1`, `b2`, `b2`
* Returns 1 values: […, `a1 == b1 && a2 == b2`

```
float64 b2 = pop2()
float64 b1 = pop2()
float64 a2 = pop2()
float64 a1 = pop2()
push(a1 == b1 && a2 == b2)
```

### Instruction `min.v2d`

Performs per component minimum calculation on the two top 64-bit floating point vector operands on the stack.

* Instruction code: `0xa8`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a1`, `a2`, `a2`, `b1`, `b1`, `b2`, `b2`
* Returns 4 values: […, `min(a1, b1)`, `min(a1, b1)`, `min(a2, b2)`, `min(a2, b2)`

```
float64 b2 = pop2()
float64 b1 = pop2()
float64 a2 = pop2()
float64 a1 = pop2()
push(min(a1, b1), min(a2, b2))
```

### Instruction `max.v2d`

Performs per component maximum calculation on the two top 64-bit floating point vector operands on the stack.

* Instruction code: `0xa9`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a1`, `a2`, `a2`, `b1`, `b1`, `b2`, `b2`
* Returns 4 values: […, `max(a1, b1)`, `max(a1, b1)`, `max(a2, b2)`, `max(a2, b2)`

```
float64 b2 = pop2()
float64 b1 = pop2()
float64 a2 = pop2()
float64 a1 = pop2()
push(max(a1, b1), max(a2, b2))
```
