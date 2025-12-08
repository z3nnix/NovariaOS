#define NULL ((void*)0) // Define the null pointer. 

unsigned int randSeed = 12345;  // Initializing the seed for the pseudo-random number generator

// generate a pseudo-random number in the range [min, max]
static inline int rand(int min, int max) {
    randSeed = randSeed * 1103515245 + 12345;
    int randValue = (randSeed / 65536) % 32768;
    
    return min + (randValue % (max - min + 1));
}

// generate a pseudo-random number in the range [min, max] with given seed [seed].
static inline int srand(int min, int max, int seed) {
    unsigned int randSeed = seed;
    randSeed = randSeed * 1103515245 + 12345;

    int randValue = (randSeed / 65536) % 32768;
    
    return min + (randValue % (max - min + 1));
}

static inline void reverse(char* str, int length) {
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

static inline char* itoa(int num, char* str, int base) {
    int i = 0;
    bool isNegative = false;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    if (num < 0 && base == 10) {
        isNegative = true;
        num = -num;
    }

    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    if (isNegative)
        str[i++] = '-';

    str[i] = '\0';

    reverse(str, i);

    return str;
}