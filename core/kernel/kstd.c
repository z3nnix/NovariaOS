#ifndef _KSTD_H
#define _KSTD_H
typedef _Bool bool;

#define true 1
#define false 0
void newline(void);
void vgaprint(const char *str, int color);

void reverse(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void strncpy(char *dest, const char *src, unsigned int n) {
    unsigned int i = 0;
    while (i < n && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = '\0';
        i++;
    }
}


char* itoa(int num, char* str, int base) {
    int i = 0;
    bool is_negative = false;

    // Handle 0 explicitly, since it is a special case
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    // Handle negative numbers only for base 10
    if (num < 0 && base == 10) {
        is_negative = true;
        num = -num;
    }

    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0'; // Convert to character
        num /= base;
    }

    // If the number is negative, append '-'
    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0'; // Null-terminate the string

    // Reverse the string
    reverse(str, i);

    return str;
}

void kprint(const char *str, int color) {
    while (*str) { // While not at the end of the string
        if (*str == '\n') {
            newline(); // Move to a new line
        } else {
            // Print a single character with the specified color
            char temp[2] = {*str, '\0'}; // Create a temporary string for one character
            vgaprint(temp, color); // Print current character with specified color
        }
        str++; // Move to the next character
    }
}

#endif // _KSTD_H