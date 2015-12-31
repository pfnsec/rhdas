#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "input.h"
#include "output.h"
#include "option.h"
#include "opcode.h"


int main(int argc, char *argv[]) {
	FILE *source;

	if(argc != 2) {
		printf("Usage: %s [file]\n", argv[0]);
		return 0;
	}

	source = fopen(argv[1], "r");

	if(!source) {
		printf("%s: Failed to open %s: %s\n", argv[0], argv[1], strerror(errno));
		return errno;
	}

	assemble(source);
}
