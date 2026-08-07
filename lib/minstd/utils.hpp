/* MinSTD provides most functions which would otherwise be found in the standard library. */
/* This here is for all the functions that do not get a dedicated file */

void srand(uint32_t seed);
int rand(void);
 
int atoi(const char *s);
void dtoa(double dbl, char *s, int s_len);
void itoa(uint32_t n, char *s);
void itohex(uint32_t n, char *s);

uint32_t ntohl(uint32_t data);
uint32_t htonl(uint32_t data);
uint16_t ntohs(uint16_t data);
uint16_t htons(uint16_t data);