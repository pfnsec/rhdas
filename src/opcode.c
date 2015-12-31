#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


int lineno = 1;


typedef enum lex_state {
	character,
	whitespace
} lex_state;


char **lex_source(FILE *source) {
	char **opcode;
	int   opcode_index = 0;

	lex_state state = whitespace;

	char *source_line = 0;
	size_t size;
	int error = 0;

	opcode = calloc(5, sizeof(char *));

	error = getline(&source_line, &size, source);


	if(error == -1) {
		free(source_line);
		free(opcode);
		return 0;
	} else if (error == 0 || source_line == 0) {
		free(source_line);
		free(opcode);
		return 0;
	} else {
		for(int i = 0; source_line[i] != 0; i++) {
			if(opcode_index > 4) {
				printf("Error: Line %d: Too many opcodes!\n", lineno);
				free(source_line);
				return 0;
			}

			switch(source_line[i]) {
				case '\t':
				case ' ' :
				case ',' :
					if(state == character)
						source_line[i] = 0;
					state = whitespace;
					continue;

				case '\n':
					source_line[i] = 0;
					return opcode;

				default: {
					if(state == whitespace) {
						opcode[opcode_index] = &source_line[i];
						opcode_index++;
						state = character;
					} else
						continue;
					
				}
			}
		}

		opcode[opcode_index] = 0;

		return opcode;
	}
}


int parse_immediate(char *source, int width, char *output, int offset, int optional) {
	unsigned imm;

	if(source == 0) {
		if(!optional) {
			printf("Error: Line %d: Expected immediate\n", lineno);
			return 1;
		} else {
			imm = 0;
			goto output;
		}
	}

	if(source[0] == '+' || source[0] == '-') {
		source++;
	}

	if(source[1] == 'x') {
		if(!(sscanf(source, "%x", &imm))) {
			printf("Error: Line %d: Expected immediate\n", lineno);
			return 1;
		}
	} else if((!sscanf(source, "%u", &imm))) {
		printf("Error: Line %d: Expected immediate\n", lineno);
		return 1;
	} else {
	}

output:

	for(int i = 0; i < width; i++) {
		if((imm) & (1 << i)) {
			output[offset + width - 1 - i] = '1';
		} else {
			output[offset + width - 1 - i] = '0';
		}
	}

	return 0;
}

void parse_sign(char *source, char *output, int sign_offset) {
	if(source == 0) {
		output[sign_offset] = '0';
		return;
	}

	if(source[0] == '+') {
		output[sign_offset] = '0';
	} else if(source[0] == '-') {
		output[sign_offset] = '1';
	} else {
		output[sign_offset] = '0';
	}

}

int parse_register(char *source, char *output, int offset) {
	unsigned reg;

	if(source == 0)
		reg = 0;

	if(!(sscanf(source, "r%u", &reg))) {
		printf("Error: Line %d: Expected register\n", lineno);
		return 1;
	}

	for(int i = 0; i < 3; i++) {
		if((reg) & (1 << i)) {
			output[offset + 2 - i] = '1';
		} else {
			output[offset + 2 - i] = '0';
		}
	}
/*
	for(int i = offset; i < offset + 3; i++) {
		if((reg >> i) & 1) {
			output[i] = '1';
		} else {
			output[i] = '0';
		}
	}
*/
	return 0;
}

int register_arg(char *arg) {
	if(arg[0] == 'r') {
		return 1;
	}

	return 0;
}

int pc_arg(char *arg) {
	if(strcmp(arg, "pc") == 0) {
		return 1;
	}

	return 0;
}

void set_opcode(const char *op, char *output) {
	strcpy(output, op);
}

void set_alu_opcode(const char *op, char *output) {
	strncpy((char *) (output + 11), op, 6);
}

void set_nop(char *output) {
	for(int i = 0; i < 16; i++) {
		output[i] = '0';
	}
}


char *output_binary(char **opcode) {
	char *output = malloc(17);

	if(strcmp(opcode[0], "nop") == 0) {
		set_nop(output);

	} else if(strcmp(opcode[0], "bx") == 0) {
		if(pc_arg(opcode[1])) {
			set_opcode("01100", output);
			parse_immediate(opcode[2], 10, output, 6, 0);
			parse_sign(opcode[2], output, 5);
		} else {
			set_opcode("0011000", output);
			parse_register(opcode[1], output, 7);
			parse_immediate(opcode[2], 5, output, 11, 1);
			parse_sign(opcode[2], output, 10);
		}

	//Load Register (PC relative or register addr)
	} else if(strcmp(opcode[0], "ldr") == 0) {
		if(pc_arg(opcode[2])) {
			set_opcode("0100", output);
			parse_register(opcode[1], output, 4);
			parse_immediate(opcode[3], 8, output, 8, 1);
			parse_sign(opcode[3], output, 7);
			
		} else {
			set_opcode("1010", output);
			parse_register(opcode[1], output, 4);
			parse_register(opcode[2], output, 7);
			parse_immediate(opcode[3], 5, output, 11, 1);
			parse_sign(opcode[3], output, 10);
		}

	//Store Register (PC relative or register addr)
	} else if(strcmp(opcode[0], "str") == 0) {
		if(pc_arg(opcode[2])) {
			set_opcode("0101", output);
			
		} else {
			set_opcode("1011", output);
			parse_register(opcode[1], output, 4);
			parse_register(opcode[2], output, 7);
			parse_immediate(opcode[3], 5, output, 11, 1);
			parse_sign(opcode[3], output, 10);
		} 

	//Register/Immediate Move
	} else if(strcmp(opcode[0], "mov") == 0) {
		if(register_arg(opcode[2])) {
			set_opcode("00101", output);
			parse_register(opcode[1], output, 5);
			parse_register(opcode[2], output, 8);
			parse_immediate(opcode[3], 4, output, 12, 1);
			parse_sign(opcode[3], output, 11);
			
		} else {
			set_opcode("100", output);
			parse_register(opcode[1], output, 3);
			parse_immediate(opcode[2], 10, output, 6, 0);
		}

	//ALU Instructions
	} else if(strcmp(opcode[0], "add") == 0) {
		set_opcode("11", output);
		parse_register(opcode[1], output, 2);
		parse_register(opcode[2], output, 5);
		parse_register(opcode[3], output, 8);
		set_alu_opcode("00000", output);

	} else if(strcmp(opcode[0], "adc") == 0) {
		set_opcode("11", output);
		parse_register(opcode[1], output, 2);
		parse_register(opcode[2], output, 5);
		parse_register(opcode[3], output, 8);
		set_alu_opcode("00001", output);

	} else if(strcmp(opcode[0], "sub") == 0) {
		set_opcode("11", output);
		parse_register(opcode[1], output, 2);
		parse_register(opcode[2], output, 5);
		parse_register(opcode[3], output, 8);
		set_alu_opcode("00010", output);

	} else if(strcmp(opcode[0], "not") == 0) {
		set_opcode("11", output);
		parse_register(opcode[1], output, 2);
		parse_register(opcode[2], output, 5);
		parse_register(opcode[3], output, 8);
		set_alu_opcode("00011", output);

	} else if(strcmp(opcode[0], "and") == 0) {
		set_opcode("11", output);
		parse_register(opcode[1], output, 2);
		parse_register(opcode[2], output, 5);
		parse_register(opcode[3], output, 8);
		set_alu_opcode("00100", output);

	} else if(strcmp(opcode[0], "or" ) == 0) {
		set_opcode("11", output);
		parse_register(opcode[1], output, 2);
		parse_register(opcode[2], output, 5);
		parse_register(opcode[3], output, 8);
		set_alu_opcode("00101", output);

	} else if(strcmp(opcode[0], "xor") == 0) {
		set_opcode("11", output);
		parse_register(opcode[1], output, 2);
		parse_register(opcode[2], output, 5);
		parse_register(opcode[3], output, 8);
		set_alu_opcode("00110", output);

	} else if(strcmp(opcode[0], "csr") == 0) {
		set_opcode("11", output);
		parse_register(opcode[1], output, 2);
		parse_register(opcode[2], output, 5);
		parse_register(opcode[3], output, 8);
		set_alu_opcode("00111", output);

	} else if(strcmp(opcode[0], "csl") == 0) {
		set_opcode("11", output);
		parse_register(opcode[1], output, 2);
		parse_register(opcode[2], output, 5);
		parse_register(opcode[3], output, 8);
		set_alu_opcode("01000", output);

	} else if(strcmp(opcode[0], "lsr") == 0) {
		set_opcode("11", output);
		parse_register(opcode[1], output, 2);
		parse_register(opcode[2], output, 5);
		parse_register(opcode[3], output, 8);
		set_alu_opcode("01001", output);

	} else if(strcmp(opcode[0], "lsl") == 0) {
		set_opcode("11", output);
		parse_register(opcode[1], output, 2);
		parse_register(opcode[2], output, 5);
		parse_register(opcode[3], output, 8);
		set_alu_opcode("01010", output);
	}  

	output[16] = 0;
	printf("%s\n", output);
	return output;
}


void assemble(FILE *source) {
	char **opcodes;

	do {

		opcodes = lex_source(source);
		if(opcodes == 0)  {
			return;
		}

		if(opcodes[0] == 0) {
			continue;
		}

		for(int i = 0; opcodes[i] != 0; i++) {
//			printf("%s, ", opcodes[i]);
		}

		output_binary(opcodes);

	//	printf("\n");

		free(opcodes[0]);
		free(opcodes);

		lineno++;
	} while(opcodes != 0);
}


