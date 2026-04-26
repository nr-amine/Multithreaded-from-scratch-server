/* Amine Nouar 12409392, Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */


#include <string.h>
char *crlf_to_lf(char *line_with_crlf)
{
	int len = strlen(line_with_crlf);
	line_with_crlf[len - 2] = '\n';
	line_with_crlf[len - 1] = '\0';
	return line_with_crlf;
}

char *lf_to_crlf(char *line_with_lf)
{
	int len = strlen(line_with_lf);
	if (len + 2 >= 512) {
		return NULL;	}
	line_with_lf[len] = '\r';
	line_with_lf[len + 1] = '\n';
	line_with_lf[len + 2] = '\0';
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