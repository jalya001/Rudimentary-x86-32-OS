void reverse(char *s);
int strlen(const char *s);

void strcpy(char *dest, char *source);
int strncmp(const char *s, const char *t, size_t n);
int strcmp(const char *s, const char *t);
char *strncpy(char *dest, const char *src, int len);
int strlcpy(char *dest, const char *src, int size);

int same_string(char *s1, char *s2); // calls strcmp

void buffer_copy(const char *source, char *destin, int size);
void buffer_zero(char *a, int size);
