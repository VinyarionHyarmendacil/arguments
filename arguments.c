// Simple command line forwarder

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	size_t idx;
	size_t len;
	size_t growth;
	char *buf;
} charbuf;

charbuf create_charbuf(size_t len, size_t growth) {
	charbuf cb = {0};
	cb.idx = 0;
	cb.len = len;
	cb.growth = growth;
	cb.buf = (char *)malloc(sizeof(char) * len);
	return cb;
}

void append_grow(charbuf *buffer, char character) {
	if (buffer->idx == buffer->len) {
		size_t newsize = buffer->len + buffer->growth;
		char *temp = (char *)malloc(buffer->len);
		for (size_t ptr = 0; ptr < buffer->len; ptr++) {
			temp[ptr] = buffer->buf[ptr];
		}
		free(buffer->buf);
		buffer->len = newsize;
		buffer->buf = temp;
	}
	buffer->buf[buffer->idx++] = character;
}

int main(size_t argc, char *argv[]) {
	// Constants
	char DQT = '\"';
	char BSL = '\\';
	char *CONFIG = "./app.cfg";
	char *FNF = "app.cfg not found, creating";
	char *FAIL = "malloc failed";
	char *R = "r";
	char *WB = "wb";
	
	// Accumulate the length of arguments, excluding the executable
	size_t len = 0;
	for (size_t i = 1; i < argc; i++) {
		len += strlen(argv[i]);
	}
	FILE *config = fopen(CONFIG, R);
	if (config == NULL) {
		// Create if absent
		printf(FNF);
		config = fopen(CONFIG, WB);
		// We aren't going to read an empty file
		if (config != NULL) {
			fclose(config);
			config = NULL;
		}
	} else {
		// Accumulate the length of the prepended arguments file
		fseek(config, 0L, SEEK_END);
		len += ftell(config);
		rewind(config);
	}
	// Buffer for system call, don't worry, it gets freed
	// +2: one separating file contents and our arguments, one null terminator
	charbuf cmd = create_charbuf(len + 2, 32);
	// Just being responsible
	if (cmd.buf == NULL) {
		printf(FAIL);
		return -1;
	}
	// Number of consecutive control characters
	size_t control = 0;
	// Copy prepended arguments with escaping
	if (config != NULL) {
		// Number of consecutive printable characters
		size_t normal = 0;
		// Number of bypassed characters
		size_t bypass = 0;
		// Read entire file
		for (char c = fgetc(config); c != EOF; c = fgetc(config)) {
			if (c < ' ') {
				// Contiguous control codes are replaced with a single space
				if (control == 0) {
					cmd.buf[cmd.idx++] = ' ';
				}
				control++;
				normal = 0;
				bypass = 0;
			} else if (bypass > 0) {
				// We are in a #comment
				normal++;
				bypass++;
			} else if (normal == 0 && bypass == 0 && c == '#') {
				// We are starting a #comment
				control = 0;
				bypass++;
			} else {
				// Normal appending arguments
				cmd.buf[cmd.idx++] = c;
				control = 0;
				normal++;
			}
		}
		// Clean up
		fclose(config);
	}
	
	// Copy our arguments to system call
	for (size_t i = 1; i < argc; i++) {
		char *in = argv[i];
		size_t l = strlen(in);
		char escqt = 0;
		append_grow(&cmd, ' ');
		for (size_t j = 0; j < l; j++) {
			if (DQT == in[j]) {
				escqt++;
				break;
			}
		}
		if (escqt != 0) {
			append_grow(&cmd, DQT);
		}
		for (size_t j = 0; j < l; j++) {
			char argchar = in[j];
			if (escqt != 0 && DQT == argchar) {
				append_grow(&cmd, BSL);
			}
			append_grow(&cmd, argchar);
		}
		if (escqt != 0) {
			append_grow(&cmd, DQT);
		}
	}
	// Null terminator fill
	while (cmd.idx < cmd.len) {
		cmd.buf[cmd.idx++] = '\0';
	}
	// Run
	int result = system(cmd.buf);
	// Clean up
	free(cmd.buf);
	return result;
}
