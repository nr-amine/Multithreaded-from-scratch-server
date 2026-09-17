#include <string.h>
char *crlf_to_lf(char *line) {
    int len = strlen(line);
    if (len < 2 || line[len-2] != '\r' || line[len-1] != '\n') return NULL;
    line[len - 2] = '\n';
    line[len - 1] = '\0';
    return line;
}

char *lf_to_crlf(char *line_with_lf) {
    int len = strlen(line_with_lf);
    if (len == 0 || len + 1 >= 512) return NULL;
    line_with_lf[len - 1] = '\r';
    line_with_lf[len]     = '\n';
    line_with_lf[len + 1] = '\0';
    return line_with_lf;
}

char *starts_with(const char *str, const char *prefix)
{
	size_t prefix_len = strlen(prefix);
	for (size_t i = 0; i < prefix_len; i++) {
		if (str[i] == '\0' || str[i] != prefix[i]) {
			return NULL;
		}
	}
	return (char *)(str + prefix_len);
}

/* These functions are imported directly from https://github.com/stedonet/chex,
   the goal is to keep using buffer_fgets, but the latter completely breaks if the message is 
   encrypted by AES since \n gets hidden, so the solution I came up with is to turn the
   normal string into a hex string and then append \n manually */

/* Copyright (c) 2022 Tero 'stedo' Liukko, MIT License */
static unsigned char chex_fromxdigit(unsigned h){
  return ((h & 0xf) + (h >> 6) * 9);
}

unsigned chex_decode(void* bin, unsigned blen, const char* hex, unsigned hlen){
  unsigned i, j;
  for(i = 0, j = 0; (i < blen) && (j+1 < hlen); ++i, j+=2){
    unsigned char hi = chex_fromxdigit(hex[j+0]);
    unsigned char lo = chex_fromxdigit(hex[j+1]);
    ((unsigned char*)bin)[i] = (hi << 4) | lo;
  }
  return i;
}

unsigned chex_isxdigit(unsigned h){
  unsigned char n09 = h - '0';
  unsigned char nAF = (h | 0x20) - 'a';
  return (n09 <= (9 - 0)) || (nAF <= (0xf - 0xa));
}
unsigned chex_encode(const void* bin, unsigned blen, char* hex, unsigned hlen){
  static const char map[] = "0123456789abcdef";
  unsigned i, j;
  const unsigned char* ubin = (const unsigned char*)bin;
  for(i = 0, j = 0; (i < blen) && (j+1 < hlen); ++i, j+=2){
    hex[j+0] = map[(ubin[i] >> 4) & 0xF];
    hex[j+1] = map[(ubin[i] >> 0) & 0xF];
  }
  if(j < hlen) hex[j] = '\0';
  return j;
}
