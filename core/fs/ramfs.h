// void* memcpy(void* dest, const void* src, size_t n) {
//     char* d = dest;
//     const char* s = src;
//     for (size_t i = 0; i < n; i++) {
//         d[i] = s[i];
//     }
//     return dest;
// }

// char* strncpy(char* dest, const char* src, size_t n) {
//     size_t i;
//     for (i = 0; i < n && src[i] != '\0'; i++) {
//         dest[i] = src[i];
//     }
//     for (; i < n; i++) {
//         dest[i] = '\0';
//     }
//     return dest;
// }

// size_t strlen(const char* str) {
//     size_t len = 0;
//     while (str[len]) {
//         len++;
//     }
//     return len;
// }

// int strcmp(const char* s1, const char* s2) {
//     while (*s1 && (*s1 == *s2)) {
//         s1++;
//         s2++;
//     }
//     return *(unsigned char*)s1 - *(unsigned char*)s2;
// }