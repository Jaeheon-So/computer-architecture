#include <stdio.h>
#include <stdlib.h>

struct cacheline
{
	unsigned int tag:19;
	unsigned int sca:1;
	unsigned int valid:1;
	unsigned int dirty:1 ;
	int data[16];
};
struct cacheline Cache[128];
int ReadMem(int addr);
void WriteMem(int addr, int value);
int hit_count = 0;
int miss_count = 0;

void load2Memory(FILE* fp);
void fetch_instruction();
void decode_instruction();
void execute_instruction();
void memory_access();
void writeback();
void update_pc();
void changed_state();
void control_signal();
void count_type();
void print_count();
void count_mem_access();

int memory[0x400000];
int REG[32] = {0, 0, 0, 0, 0, 0, 0, 0,
	       0, 0, 0, 0, 0, 0, 0, 0,
	       0, 0, 0, 0, 0, 0, 0, 0,
	       0, 0, 0, 0, 0, 0x1000000, 0, 0xffffffff};
int PC = 0;
int instruction;
int RegDst, Jump, JumpReg, Branch, MemRead, MemtoReg, ALUOp, MemWrite, ALUSrc, RegWrite;
int opcode, rs, rt, rd, imm, shamt, funct, address, simm, zimm, j_address, b_address;
int write_Reg, ALU_data1, ALU_data2, ALU_result, bcond, mem_value;
int instruction_count = 0; int r_count = 0; int i_count = 0; 
int j_count = 0; int mem_count = 0; int branch_count = 0;

int main(int argc, char* argv[])
{
	FILE* fp;
	if(argc == 2)
		fp = fopen(argv[1], "rb");	
	else
		fp = fopen("simple.bin", "rb");
	load2Memory(fp);
	fclose(fp);
	while(1)
	{
		if(PC == 0xffffffff)
		{
			printf("PC = 0xffffffff ---> Halt Program\n\n");
			break;
		}
		fetch_instruction();
		decode_instruction();
		execute_instruction();
		memory_access();
		writeback();
		update_pc();
		changed_state();
	}
	printf("Final Return Value REG[2] = 0x%x\n", REG[2]);
	print_count();
	return 0;
}

int ReadMem(int addr)
{
	int tag = addr >> 13;
	int index = (addr >> 6) & 0x7f;
	int offset = addr & 0x3f;
	int start_addr = addr & 0xffffffc0;
	int wstart_addr = (Cache[index].tag << 13) | (index << 6);
	int i;
	if(Cache[index].valid)
	{
		if(Cache[index].tag == tag) // cache hit!!
		{
			hit_count++;
			printf("CA : ReadMem Cache hit!!(0x%x)\n", addr);
			return Cache[index].data[offset/4];
		}
		else // conflict miss
		{
			miss_count++;
			printf("MA : ReadMem Conflict miss!!(0x%x)\n", addr);
			if(Cache[index].dirty) //if dirty, update memory before erase
			{
				for(i = 0; i < 16; i++)
				{
					memory[(wstart_addr + 4*i) / 4] = Cache[index].data[i];
				}
				printf("   : memory[0x%x ~ 0x%x] = Cache[0x%x].data[0 ~ 63]\n", (wstart_addr), (wstart_addr + 4*16)-1, index);
				Cache[index].dirty = 0;
			}
			Cache[index].tag = tag;
			for(i = 0; i < 16; i++)
			{
				Cache[index].data[i] = memory[(start_addr + 4*i) / 4];
		        }
			printf("   : Cache[0x%x].data[0 ~ 63] = memory[0x%x ~ 0x%x]\n", index, start_addr,  (start_addr + 4*16)-1);	
			return Cache[index].data[offset/4];	
		}
	}
	else // cold miss
	{
		miss_count++;
		printf("MA : ReadMem Cold miss!!(0x%x)\n", addr);
		for(i = 0; i < 16; i++)
		{
			Cache[index].data[i] = memory[(start_addr + 4*i) / 4];
		}
		printf("   : Cache[0x%x].data[0 ~ 63] = memory[0x%x ~ 0x%x]\n", index, start_addr,  (start_addr + 4*16)-1);
		Cache[index].valid = 1;
		Cache[index].tag = tag;
		return Cache[index].data[offset/4];
	}
}

void WriteMem(int addr, int value)
{
	int tag = addr >> 13; 
	int index = (addr >> 6) & 0x7f;
	int offset = addr & 0x3f;
	int start_addr = addr & 0xffffffc0;
	int wstart_addr = (Cache[index].tag << 13) | (index << 6);
	int i;
	if(Cache[index].valid) 
	{
		if(Cache[index].tag == tag) // cache hit!!
		{
			hit_count++;
			printf("CA : WriteMem Cache hit!!(0x%x)\n", addr);
			Cache[index].data[offset/4] = value;
			printf("   : Cache[0x%x].data[%d] = %d\n", index, offset, value);
			Cache[index].dirty = 1;	
		}
		else  // conflict miss
		{
			miss_count++;
			printf("MA : WriteMem Conflict miss!!(0x%x)\n", addr);
			if(Cache[index].dirty)
	      		{
				for(i = 0; i < 16; i++)
				{
					memory[(wstart_addr + 4*i) / 4] = Cache[index].data[i];
				}
				printf("   : memory[0x%x ~ 0x%x] = Cache[0x%x].data[0 ~ 63]\n", (wstart_addr), (wstart_addr + 4*16)-1, index);
			}
			Cache[index].tag = tag;
			for(i = 0; i < 16; i++)
	       		{
   				Cache[index].data[i] = memory[(start_addr + 4*i) / 4];
			}
			printf("   : Cache[0x%x].data[0 ~ 63] = memory[0x%x ~ 0x%x]\n", index, start_addr,  (start_addr + 4*16)-1);	
			Cache[index].data[offset/4] = value;
			printf("CA : Cache[0x%x].data[%d] = %d\n", index, offset, value);
			Cache[index].dirty = 1;	
		}
	}
	else // cold miss
	{
		 miss_count++;
		 printf("MA : WriteMem Cold miss!!(0x%x)\n", addr);
		 for(i = 0; i < 16; i++)
		 {	
			 Cache[index].data[i] = memory[(start_addr + 4*i) / 4];
		 }
		 printf("   : Cache[0x%x].data[0 ~ 63] = memory[0x%x ~ 0x%x]\n", index, start_addr,  (start_addr + 4*16)-1);
                 Cache[index].data[offset/4] = value;
		 printf("CA : Cache[0x%x].data[%d] = %d\n", index, offset, value);		 
	         Cache[index].valid = 1;
		 Cache[index].dirty = 1;
                 Cache[index].tag = tag;
	}
}

void load2Memory(FILE* fp)
{
	char buffer[4];
	int temp;
	int index = 0;
	int* data = &buffer[0];
	printf("-----------------------------------------------------------------\n");
	while(1)
	{	
		fread(buffer, sizeof(buffer), 1, fp);
		if(feof(fp) == 1)
			break;
		printf("Data : %08x\t\t", *data);

		temp = buffer[3];
		buffer[3] = buffer[0];
		buffer[0] = temp;
		temp = buffer[2];
		buffer[2] = buffer[1];
		buffer[1] = temp;

		memory[index/4] = *data;
		printf("Instruction Memory : [%08x]  %08x\n", index, memory[index/4]);
		index = index + 4;
	}
	printf("-----------------------------------------------------------------\n");
	return;
}

void fetch_instruction()
{
	instruction = ReadMem(PC);
	printf("--------Instruction %d--------PC : 0x%x\n", instruction_count + 1, PC);
	printf("FI : 0x%08x\n", instruction);
	instruction_count++;
	return;
}

void decode_instruction()
{
	opcode = (instruction & 0xfc000000) >> 26;
	rs = (instruction & 0x03e00000) >> 21;
	rt = (instruction & 0x001f0000) >> 16;
	rd = (instruction & 0x0000f800) >> 11;
	imm = (instruction & 0x0000ffff);
	shamt = (instruction & 0x000007c0) >> 6;
	funct = (instruction & 0x0000003f);
	address = (instruction & 0x03ffffff);
	simm = (imm << 16) >> 16;
	zimm = imm;
	j_address = address << 2;
	b_address = (imm << 16) >> 14;

	if(opcode == 0)  // R type
	{
		printf("DI : opcode = 0x%x, rs = %d, rt = %d, rd = %d, shamt = %d, funct = 0x%x\n", 
				opcode, rs, rt, rd, shamt, funct);
	}
	else if(opcode == 0x2 || opcode == 0x3)   // j, jal
	{
		printf("DI : opcode = 0x%x\t Jump Address = 0x%x\n", opcode, j_address);
	}
	else   // I type
	{
		printf("DI : opcode = 0x%x, rs = %d, rt = %d, imm = %d, simm = %d\n", 
				opcode, rs, rt, imm, simm);
	}
	control_signal();
	count_type();
	count_mem_access();
	return;
}

void execute_instruction()
{
	if(Jump) // J, JAL
		return;
	if(JumpReg) // JR, JALR
		return;

	ALU_data1 = REG[rs];
	if(ALUSrc)  // I type except beq, bne
	{
		if(opcode == 0xc || opcode == 0xd) // andi, ori
			ALU_data2 = zimm;
		else if(opcode == 0xf) // lui
			ALU_data2 = imm;
		else  // I type 나머지
			ALU_data2 = simm;
	}
	else  // R type + bne, beq
	{
		if(opcode == 0 && (funct == 0 || funct == 0x2)) // sll, srl
			ALU_data1 = shamt;
		ALU_data2 = REG[rt];
	}
	
	switch(ALUOp)
	{
		case 0x20:   // add, lw, sw
		case 0x8:    // addi
		case 0x9:    // addiu
		case 0x21:   // addu
			ALU_result = ALU_data1 + ALU_data2;
			printf("EI : ALU Result = 0x%x + 0x%x = 0x%x\n",
				        ALU_data1, ALU_data2, ALU_result);
			break;
		case 0x24:   // and
		case 0xc:    // andi
			ALU_result = ALU_data1 & ALU_data2;
			printf("EI : ALU Result = 0x%x & 0x%x = 0x%x\n",
				       	ALU_data1, ALU_data2, ALU_result);
			break;
		case 0x4:    // beq
			if(ALU_data1 == ALU_data2)
				bcond = 1;
			printf("EI : bcond = %d\n", bcond);
			break;
		case 0x5:    // bne
			if(ALU_data1 != ALU_data2)
				bcond = 1;
			printf("EI : bcond = %d\n", bcond);
			break;
		case 0xf:    // lui
			ALU_result = ALU_data2 << 16;
			printf("EI : ALU Result = 0x%x << 16 = 0x%x\n", 
					ALU_data2, ALU_result);
			break;
		case 0x27:   // Nor
			ALU_result = ~(ALU_data1 | ALU_data2);
			printf("EI : ALU Result = ~(0x%x | 0x%x) = 0x%x\n", 
					ALU_data1, ALU_data2, ALU_result);
			break;
		case 0x25:   // or
		case 0xd:    // ori
			ALU_result = ALU_data1 | ALU_data2;
			printf("EI : ALU Result = 0x%x | 0x%x = 0x%x\n", 
					ALU_data1, ALU_data2, ALU_result);
			break;
		case 0x2a:   // slt
		case 0xa:    // slti
		case 0xb:    // sltiu
		case 0x2b:   // sltu
			ALU_result = (ALU_data1 < ALU_data2) ? 1 : 0;
			printf("EI : ALU Result = (0x%x < 0x%x) ? 1 : 0 = 0x%x\n",
				       	ALU_data1, ALU_data2, ALU_result);
			break;
		case 0x00:   // sll
			ALU_result = ALU_data2 << ALU_data1;
			printf("EI : ALU Result = 0x%x << %d = 0x%x\n", 
					ALU_data1, ALU_data2, ALU_result);
			break;
		case 0x02:   // srl
			ALU_result = ALU_data2 >> ALU_data1;
			printf("EI : ALU Result = 0x%x >> 0x%x = 0x%x\n", 
					ALU_data1, ALU_data2, ALU_result);
			break;
		case 0x22:   // sub
		case 0x23:   // subi
			ALU_result = ALU_data1 - ALU_data2;
			printf("EI : ALU Result = 0x%x - 0x%x = 0x%x\n", 
					ALU_data1, ALU_data2, ALU_result);
			break;
		default:     // 없는 명령어
			printf("No Such Instruction\n");
			exit(1);
	}
	return;
}

void memory_access()
{
	if(MemRead) // LW
	{
		mem_value = ReadMem(ALU_result);
	}
	if(MemWrite)  // SW
	{
		WriteMem(ALU_result, REG[rt]);
		//printf("MA : Memory[0x%x] = REG[%d](0x%x)\n", ALU_result, rt, REG[rt]);
	}
	return;
}

void writeback()
{
	if(RegDst)
		write_Reg = rd;	
	else
		write_Reg = rt;
	
	if(RegWrite)
	{
		if(Jump) // JAL
		{
			REG[31] = PC + 8;
			printf("WB : REG[31] = PC+8(0x%x)\t", PC+8);
		}
		else if(JumpReg)  // JALR
		{
			REG[write_Reg] = PC + 4;
			printf("WB : REG[%d] = PC+4(0x%x)\t", 
					write_Reg, PC+4);
		}
		else if(MemtoReg)  // LW
		{
			REG[write_Reg] = mem_value;
			printf("WB : REG[%d] = Cache[0x%x].data[%d](0x%x)\n",
				      write_Reg, (ALU_result >>6) & 0x7f, (ALU_result & 0x3f),  mem_value);	
		}
		else
		{
			REG[write_Reg] = ALU_result; 
			printf("WB : REG[%d] = 0x%x\n", 
					write_Reg, ALU_result);	
		}
	}
	return;
}

void update_pc()
{
	if(Jump) // J, JAL
	{
		PC = j_address; 
		printf("Jump to 0x%x\n", j_address);
	}
	else if(JumpReg) // JR, JALR
	{
		PC = REG[rs];
		printf("Jump to REG[%d](0x%x)\n", rs, REG[rs]);
	}
	else if(bcond) // beq, bne
	{
		PC = PC + 4 + b_address;
		printf("Branch taken, Jump to 0x%x\n", PC);
		branch_count++;
	}
	else
	{
		PC = PC + 4;
	}
	return;
}

void changed_state()
{
	if(RegWrite)
	{	if(Jump) // JAL
		{
			printf("Changed Register : REG[31] = PC+8(0x%x)\n", 
					REG[31]);
		}
		else if(JumpReg) // JALR
		{
			printf("Changed Register : REG[%d] = PC+4(0x%x)\n", 
					rd, REG[rd]);
		}
		else
		{
			printf("Changed Register : REG[%d] = 0x%x\n", 
					write_Reg, REG[write_Reg]);
		}
	}
	/*
	if(MemWrite) // SW
	{
		printf("Changed Memory : Memory[0x%x] = 0x%x\n", 
				ALU_result, REG[rt]);
	}*/
	printf("Changed PC = 0x%x\n\n", PC);
	return;
}

void control_signal()
{
	RegDst = 0; Jump = 0; JumpReg = 0; Branch = 0; MemRead = 0; 
	MemtoReg = 0; ALUOp = 0; MemWrite = 0; ALUSrc = 0; RegWrite = 0;
	bcond = 0;
	if(opcode == 0)  // R type
	{	if(funct == 0x8 || funct == 0x9)  // JR, JALR
		{
			if(funct == 0x9) // JALR
			{
				RegWrite = 1;
				RegDst = 1;
			}	
			JumpReg = 1;
		}
		else
		{
			RegDst = 1;
			RegWrite = 1;
			ALUOp = funct;
		}
	}
	else if(opcode == 0x23) // LW
	{
		RegWrite = 1;
		MemRead = 1;
		MemtoReg = 1;
		ALUSrc = 1;
		ALUOp = 0x20;   
	}
	else if(opcode == 0x2b)  // SW
	{
		MemWrite = 1;
		ALUSrc = 1;
		ALUOp = 0x20;
	}
	else if(opcode == 0x2 || opcode == 0x3) // j, jal
	{
		if(opcode == 0x3) // jal
			RegWrite = 1;
		Jump = 1;
	}
	else if(opcode == 0x4 || opcode == 0x5) // beq, bne
	{
		Branch = 1;
		ALUOp = opcode;	
	}
	else // I type
	{
		RegWrite = 1;
		ALUSrc = 1;
		ALUOp = opcode;
	}
	return;
}
void count_type()
{
	if(opcode == 0)
	{
		r_count++;
	}
	else if(opcode == 0x2 || opcode == 0x3)
		j_count++;
	else
		i_count++;
	return;	
}

void count_mem_access()
{
	if(opcode == 0x23 || opcode == 0x2b)
		mem_count++;
	return;
}

void print_count()
{
	double hit_rate = (double)hit_count /(hit_count + miss_count);
	double miss_rate = (double)miss_count / (hit_count + miss_count);
	printf("Instruction count                : %d\n", instruction_count);
	printf("R_type_count                     : %d\n", r_count);
	printf("I_type_count                     : %d\n", i_count);
	printf("J_type_count                     : %d\n", j_count);
	printf("Load_Store_count                 : %d\n", mem_count);
	printf("Branch_taken_count               : %d\n", branch_count);
	printf("---------------------------------------------\n");
	printf("Hit_count                        : %d\n", hit_count);
	printf("Miss_count                       : %d\n", miss_count);
	printf("Cache_Hit_Rate                   : %f\n", hit_rate);
	printf("Cache_Miss_Rate                  : %f\n", miss_rate);
	printf("Average Memory Access Time(AMAT) : Hit Time + %f*Miss Penalty\n", miss_rate);
	printf("---------------------------------------------\n");
	return;
}
