#include <core/fs/procfs.h>
#include <core/fs/vfs.h>
#include <core/kernel/vge/fb_render.h>
#include <core/arch/cpuid.h>
#include <core/kernel/kstd.h>
#include <core/kernel/mem.h>
#include <stdint.h>
#include <string.h>

void procfs_init() {
    vfs_pseudo_register("/proc/cpuinfo", procfs_cpuinfo, NULL, NULL, NULL, NULL);
    vfs_pseudo_register("/proc/meminfo", procfs_meminfo, NULL, NULL, NULL, NULL);
    vfs_pseudo_register("/proc/pci", procfs_pci, NULL, NULL, NULL, NULL);
    vfs_pseudo_register("/proc/uptime", procfs_uptime, NULL, NULL, NULL, NULL);
    cpuinfo_init();
}

vfs_ssize_t procfs_cpu(vfs_file_t* file, void* buf, size_t count, vfs_off_t* pos) {
    kprint("Work", 3);
    return 0;
}

vfs_ssize_t procfs_meminfo(vfs_file_t* file, void* buf, size_t count, vfs_off_t* pos) {
    static char meminfo_buf[512];
    static int meminfo_initialized = 0;

    if (!meminfo_initialized) {
        size_t memTotal = getMemTotal();
        size_t memFree = getMemFree();
        size_t memUsed = memTotal - memFree;

        char total_str[32], used_str[32], free_str[32];
        formatMemorySize(memTotal, total_str);
        formatMemorySize(memUsed, used_str);
        formatMemorySize(memFree, free_str);

        strcpy_safe(meminfo_buf, "MemTotal       : ", sizeof(meminfo_buf));
        strcat_safe(meminfo_buf, total_str, sizeof(meminfo_buf));
        strcat_safe(meminfo_buf, "\nMemUsed        : ", sizeof(meminfo_buf));
        strcat_safe(meminfo_buf, used_str, sizeof(meminfo_buf));
        strcat_safe(meminfo_buf, "\nMemFree        : ", sizeof(meminfo_buf));
        strcat_safe(meminfo_buf, free_str, sizeof(meminfo_buf));
        strcat_safe(meminfo_buf, "\n", sizeof(meminfo_buf));

        meminfo_initialized = 1;
    }

    size_t len = strlen(meminfo_buf);
    if (*pos >= len) {
        return 0;
    }

    size_t remaining = len - *pos;
    size_t to_copy = (remaining < count) ? remaining : count;

    memcpy(buf, meminfo_buf + *pos, to_copy);
    *pos += to_copy;

    return to_copy;
}

vfs_ssize_t procfs_pci(vfs_file_t* file, void* buf, size_t count, vfs_off_t* pos) {
    return 0;
}

vfs_ssize_t procfs_uptime(vfs_file_t* file, void* buf, size_t count, vfs_off_t* pos) {
    return 0;
}

static char cpuinfo_buf[2048];
static int cpuinfo_initialized = 0;

vfs_ssize_t procfs_cpuinfo(vfs_file_t* file, void* buf, size_t count, vfs_off_t* pos) {
    if (!cpuinfo_initialized) {
        cpuinfo_init();
        cpuinfo_initialized = 1;
    }

    size_t len = strlen(cpuinfo_buf);
    if (*pos >= len) {
        return 0;
    }

    size_t remaining = len - *pos;
    size_t to_copy = (remaining < count) ? remaining : count;

    memcpy(buf, cpuinfo_buf + *pos, to_copy);
    *pos += to_copy;

    return to_copy;
}

void strcpy_safe(char *dest, const char *src, size_t max_len) {
    size_t i = 0;
    while (i < max_len - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void strcat_safe(char *dest, const char *src, size_t max_len) {
    size_t dest_len = strlen(dest);
    size_t i = 0;
    while (dest_len + i < max_len - 1 && src[i] != '\0') {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
}

void cpuinfo_init(void) {
    char *buf = cpuinfo_buf;
    size_t remaining = sizeof(cpuinfo_buf);

    cpuid_result_t result;

    // Get vendor ID
    cpuid(0, 0, &result);
    char vendor[13];
    memcpy(vendor, &result.ebx, 4);
    memcpy(vendor + 4, &result.edx, 4);
    memcpy(vendor + 8, &result.ecx, 4);
    vendor[12] = '\0';

    strcpy_safe(buf, "vendor_id       : ", remaining);
    strcat_safe(buf, vendor, remaining);
    strcat_safe(buf, "\n", remaining);
    buf += strlen(buf);
    remaining = sizeof(cpuinfo_buf) - (buf - cpuinfo_buf);

    // Get processor info
    cpuid(1, 0, &result);

    char num_str[16];

    strcpy_safe(buf, "cpu family      : ", remaining);
    itoa((result.eax >> 8) & 0xF, num_str, 10);
    strcat_safe(buf, num_str, remaining);
    strcat_safe(buf, "\n", remaining);
    buf += strlen(buf);
    remaining = sizeof(cpuinfo_buf) - (buf - cpuinfo_buf);

    strcpy_safe(buf, "model           : ", remaining);
    itoa((result.eax >> 4) & 0xF, num_str, 10);
    strcat_safe(buf, num_str, remaining);
    strcat_safe(buf, "\n", remaining);
    buf += strlen(buf);
    remaining = sizeof(cpuinfo_buf) - (buf - cpuinfo_buf);

    strcpy_safe(buf, "stepping        : ", remaining);
    itoa(result.eax & 0xF, num_str, 10);
    strcat_safe(buf, num_str, remaining);
    strcat_safe(buf, "\n", remaining);
}