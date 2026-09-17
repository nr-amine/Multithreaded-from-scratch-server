#include "buffer.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct buffer {
	int fd;
	char *buff;
	size_t buffsz;
	size_t start;
	size_t end;
};

buffer *buff_create(int fd, size_t buffsz)
{
	struct buffer *buff = malloc(sizeof(struct buffer));
	if (buff == NULL) {
		return NULL;
	}
	buff->fd = fd;
	buff->buff = malloc(buffsz);
	if (buff->buff == NULL) {
		free(buff);
		return NULL;
	}
	buff->buffsz = buffsz;
	buff->start = 0;
	buff->end = 0;
	return buff;
}

int buff_getc(buffer *b)
{
	if (b == NULL) {
		return EOF;
	}
	if (b->start >= b->end) {
		ssize_t sz = read(b->fd, b->buff, b->buffsz);
		if (sz <= 0) {
			return EOF;
		}
		b->start = 0;
		b->end = sz;
	}
	return b->buff[b->start++];
}

int buff_ungetc(buffer *b, int c)
{
	if (b == NULL || c == EOF || b->start == 0) {
		return EOF;
	}
	b->buff[--b->start] = (char)c;
	return c;
}

void buff_free(buffer *b)
{
	if (b == NULL) {
		return;
	}
	free(b->buff);
	free(b);
}

int buff_eof(const buffer *buff)
{
	if (buff->start >= buff->end) {
		return 1;
	}
	return 0;
}

int buff_ready(const buffer *buff)
{
	return buff->start < buff->end;
}

char *buff_fgets(buffer *b, char *dest, size_t size)
{
	for (size_t i = 0; i < size - 1; i++) {
		int c = buff_getc(b);
		if (c == EOF) {
			if (i == 0) {
				return NULL;
			}
			dest[i] = '\0';
			return dest;
		}
		dest[i] = (char)c;
		if (c == '\n') {
			dest[i + 1] = '\0';
			return dest;
		}
	}
	dest[size - 1] = '\0';
	return dest;
}

char *buff_fgets_crlf(buffer *b, char *dest, size_t size)
{
	for(size_t i = 0; i < size - 1; i++) {
		int c = buff_getc(b);
		if (c == EOF) {
			if (i == 0) {
				return NULL;
			}
			dest[i] = '\0';
			return dest;
		}
		dest[i] = (char)c;
		if (c == '\r') {
			int next_c = buff_getc(b);
			if (next_c == '\n') {
				dest[i + 1] = '\n';
				dest[i + 2] = '\0';
				return dest;
			} else if (next_c != EOF) {
				buff_ungetc(b, next_c);
			}
		}
	}
	dest[size - 1] = '\0';
	return dest;
}