#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>

#define BYTE unsigned char
#define NULLBYTE 0
#define STEP tce->cpu->pc++
#define FETCH_PROG fetch(tce->prog_mem, tce->cpu->pc);	

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

/* OP CODES
 *
 * | BINARY   |DEZ| DESCRIPTIONS             | ARGUMENTS                      |
 * +----------+---+--------------------------+--------------------------------+
 * | 0000 0000 000 HALT                      ;
 * | 0000 0001 001 MOVE CONSTANT TO DATA MEM ; %CONSTANT %ADDR
 * | 0000 0010 002 MOVE CONSTANT TO A        ; %CONSTANT
 * | 0000 0011 003 MOVE CONSTANT TO X        ; %CONSTANT
 * | 0000 0100 004 MOVE CONSTANT TO Y        ; %CONSTANT
 * | 0000 0101 005 Jump If Zero Constant     ; %CONSTANT %ADDR
 * | 0000 0110 006 Jump If Zero Register     ; %REGISTER %ADDR
 * | 0000 0111 007 ADD CONSTANT TO A         ; %VALUE
 * | 0000 1000 008 SUB CONSTANT FROM A       ; %VALUE
 * | 0000 1001 009 
 *
 */

typedef struct {
	size_t size;
	BYTE* mem;
	int cs;
} MEM;

typedef struct {
	int pc;
    	BYTE a;
    	BYTE x;	
	BYTE y;
} CPU;

typedef struct {
	CPU* cpu;
	MEM* prog_mem;
	MEM* data_mem;
} TCE;

/* MEMORY FUNCTIONS */
MEM* new_MEM(size_t size) {
	MEM* mem = malloc(sizeof(MEM));
	mem->size = size;
	BYTE* mem_array = malloc(sizeof(BYTE) * size);
	mem->mem = mem_array;
	mem->cs = 0;

	for(int i = 0; i < size; i++) mem->mem[i] = 0;
	
	return mem;
}

BYTE fetch(MEM* mem, BYTE addr) {
	return mem->mem[addr];
}

BYTE store(MEM* mem, BYTE addr, BYTE data) {
	mem->mem[addr] = data;
	return NULLBYTE;
}

BYTE handle_mem(MEM* mem, int cs, BYTE addr, BYTE data) {
	if(cs) return fetch(mem, addr);
	return store(mem, addr, data);
}

void print_mem(MEM* mem) {
	for(int y = 0; y < mem->size; y += 16) {
		printf("%02x:", y);
		for(int x = y; x < (y + 16); x++) {
			printf(" %02x", mem->mem[x]);
		}
		printf("\n");
	}
}

void cpu_status(TCE* tce) {
	printf("CPU:\nPC = %02x\nA  = %02x\nX  = %02x\nY  = %02x\n\n", tce->cpu->pc, tce->cpu->a, tce->cpu->x, tce->cpu->y);
}

void mem_status(TCE* tce) {
	int mem_alloc_prog = 0;
	for(int i = 0; i < tce->prog_mem->size; i++) {
		if(tce->prog_mem->mem[i] == 0) mem_alloc_prog++;
	}

	int alloc_prog = 256 - mem_alloc_prog;

	double prog_pro = (alloc_prog / 256 * 1.0) * 100.0;

	printf("PROGRAM MEMORY:\nALLOCATED: %d Byte (%.2f%%)\n\n", alloc_prog, prog_pro);
	print_mem(tce->prog_mem);
	printf("\n");
	
	int mem_alloc_data = 0;
	for(int i = 0; i < tce->data_mem->size; i++) {
		if(tce->data_mem->mem[i] == 0) mem_alloc_data++;
	}
	int alloc_data = 256 - mem_alloc_data;
	
	double data_pro = (alloc_data / 256 * 1.0) * 100.0;

	printf("DATA MEMORY:\nALLOCATED: %d Byte (%.2f%%)\n\n", alloc_data, data_pro);
	print_mem(tce->data_mem);
	printf("\n");
}

/* CPU FUNCTIONS */
CPU* new_CPU() {
	CPU* cpu = malloc(sizeof(CPU));
	cpu->pc = 0;
	cpu->a = 0;
	cpu->x = 0;
	cpu->y = 0;

	return cpu;
}

/* TCE FUNCTIONS */
TCE* new_TCE(CPU* cpu, MEM* prog_mem, MEM* data_mem) {
	TCE* tce = malloc(sizeof(TCE));
	tce->cpu = cpu;
	tce->prog_mem = prog_mem;
	tce->data_mem = data_mem;

	return tce;
}

BYTE cpu_fetch(TCE* tce) {
	return fetch(tce->prog_mem, tce->cpu->pc);
}

void move_const_to_data(TCE* tce) {
	STEP;
	BYTE constant = FETCH_PROG; 
	STEP;
	BYTE addr = FETCH_PROG;
	
	tce->data_mem->mem[addr] = constant;

	printf("%02x "BYTE_TO_BINARY_PATTERN" %03d MVCD %02x %02x\n", tce->cpu->pc, BYTE_TO_BINARY(1), 1, constant, addr);
}

void move_const_to_a(TCE* tce) {
	STEP;
	BYTE constant = FETCH_PROG;
	
	tce->cpu->a = constant;
	tce->cpu->pc++;

	printf("%02x "BYTE_TO_BINARY_PATTERN" %03d MVCA %02x\n", tce->cpu->pc, BYTE_TO_BINARY(2), 2, constant);
}

void jump_if_zero(TCE* tce) {
	tce->cpu->pc++;
	BYTE value = fetch(tce->prog_mem, tce->cpu->pc);
	tce->cpu->pc++;
	BYTE addr = fetch(tce->prog_mem, tce->cpu->pc);
	
	if(value == 0) {
		tce->cpu->pc = addr;
	} else {
		tce->cpu->pc++;
	}

	printf("%02x "BYTE_TO_BINARY_PATTERN" %03d JMPZ %02x %02x\n", tce->cpu->pc, BYTE_TO_BINARY(5), 5, value, addr);
}

void jump_if_zero_r(TCE* tce) {
	tce->cpu->pc++;
	BYTE reg = fetch(tce->prog_mem, tce->cpu->pc);
	tce->cpu->pc++;
	BYTE addr = fetch(tce->prog_mem, tce->cpu->pc);

	if(reg == 0 && tce->cpu->a == 0 ||
	   reg == 1 && tce->cpu->x == 0 ||
	   reg == 2 && tce->cpu->y == 0
	  ) {
		tce->cpu->pc = addr;
	} else {
		tce->cpu->pc++;
	}

	printf("%02x "BYTE_TO_BINARY_PATTERN" %03d JMPZ %02x %02x\n", tce->cpu->pc, BYTE_TO_BINARY(5), 5, reg, addr);
}

void add_constant_to_a(TCE* tce) {

}

void sub_constant_from_a(TCE* tce) {
	STEP;
	BYTE constant = FETCH_PROG;
	tce->cpu->a - constant;

	tce->cpu->pc++;

	printf("%02x "BYTE_TO_BINARY_PATTERN" %03d SUBA %02x\n", tce->cpu->pc, BYTE_TO_BINARY(8), 8, constant);
}

void execute(TCE* tce, BYTE command) {
	switch(command) {
		case 1: move_const_to_data(tce); break;
		case 2: move_const_to_a(tce); break;
		case 3: ; break;
		case 4: ; break;
		case 5: jump_if_zero(tce); break;		
		case 6: jump_if_zero_r(tce); break;
		case 7: ; break;
		case 8: sub_constant_from_a(tce); break;
	}
}

void run(TCE* tce) {
	
	printf("PC BINARY   DEZ CMD  ARGS\n");
	while(tce->cpu->pc < tce->prog_mem->size - 1 && cpu_fetch(tce) != 0) {
		BYTE command = fetch(tce->prog_mem, tce->cpu->pc);

		execute(tce, command);

		sleep(1);
	}

	printf("%02x "BYTE_TO_BINARY_PATTERN" %03d HALT\n", tce->cpu->pc, BYTE_TO_BINARY(0), 0);
}

int main(int argc, char** argv) {
	MEM* program_memory = new_MEM(256);
	MEM* data_memory = new_MEM(256);
	CPU* cpu = new_CPU();

	TCE* tce = new_TCE(cpu, program_memory, data_memory);

	/*
	 *
	 * MVCA 16
	 * SUBA 1
	 * JMPZR 0 0
	 *
	 */

	BYTE prog[256] = {2, 16, 
	     8, 1, 
	     6, 0, 2};

	for(int i = 0; i < 256; i++) {
		program_memory->mem[i] = prog[i];
	}
	
	cpu_status(tce);
        mem_status(tce);

	run(tce);

	free(program_memory);
	free(data_memory);
	free(cpu);
	free(tce);

	return 0;
}
