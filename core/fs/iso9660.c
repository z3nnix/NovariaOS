// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/fs/iso9660.h>
#include <core/kernel/kstd.h>
#include <core/kernel/mem.h>

static void* iso_data = NULL;
static size_t iso_data_size = 0;
static iso9660_pvd_t* primary_volume = NULL;
static uint16_t block_size = 2048;
static bool initialized = false;

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static void* read_block(uint32_t lba) {
    if (!iso_data || lba * block_size >= iso_data_size) {
        return NULL;
    }
    return (void*)((uint8_t*)iso_data + lba * block_size);
}

static void normalize_filename(const char* iso_name, size_t iso_len, char* out, size_t out_size) {
    size_t i;
    for (i = 0; i < iso_len && i < out_size - 1; i++) {
        if (iso_name[i] == ';') break;
        out[i] = iso_name[i];
    }
    out[i] = '\0';
}

static void to_lowercase(char* str) {
    for (size_t i = 0; str[i]; i++) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = str[i] + ('a' - 'A');
        }
    }
}

static iso9660_dir_entry_t* find_entry_in_dir(uint32_t dir_extent, uint32_t dir_size, const char* name) {
    uint8_t* dir_data = (uint8_t*)read_block(dir_extent);
    if (!dir_data) return NULL;
    
    size_t offset = 0;
    while (offset < dir_size) {
        iso9660_dir_entry_t* entry = (iso9660_dir_entry_t*)(dir_data + offset);
        if (entry->length == 0) break;
        
        char* entry_name = (char*)(entry + 1);
        uint8_t name_len = entry->name_len;
        
        if (name_len == 1 && (entry_name[0] == 0 || entry_name[0] == 1)) {
            offset += entry->length;
            continue;
        }
        
        char normalized[256];
        normalize_filename(entry_name, name_len, normalized, sizeof(normalized));
        
        if (strcmp(normalized, name) == 0) {
            return entry;
        }
        
        offset += entry->length;
    }
    
    return NULL;
}

void iso9660_init(void* iso_start, size_t iso_size) {
    iso_data = iso_start;
    iso_data_size = iso_size;
    
    for (int i = 16; i < 32; i++) {
        iso9660_pvd_t* vd = (iso9660_pvd_t*)read_block(i);
        if (!vd) return;
        
        if (vd->type == 1 && strncmp(vd->identifier, "CD001", 5) == 0) {
            primary_volume = vd;
            block_size = vd->logical_block_size_le;
            initialized = true;
            return;
        }
        
        if (vd->type == 255) break;
    }
}

const void* iso9660_find_file(const char* path, size_t* size) {
    if (!initialized || !primary_volume) {
        if (size) *size = 0;
        return NULL;
    }
    
    iso9660_dir_entry_t* root = (iso9660_dir_entry_t*)primary_volume->root_directory_entry;
    uint32_t current_extent = root->extent_le;
    uint32_t current_size = root->size_le;
    
    char path_copy[256];
    size_t path_len = strlen(path);
    if (path_len >= sizeof(path_copy)) {
        if (size) *size = 0;
        return NULL;
    }
    
    size_t j = 0;
    for (size_t i = 0; i < path_len; i++) {
        if (path[i] == '/' && j > 0 && path_copy[j-1] == '/') continue;
        path_copy[j++] = path[i];
    }
    path_copy[j] = '\0';
    
    char* search_path = path_copy;
    if (search_path[0] == '/') search_path++;
    
    if (search_path[0] == '\0') {
        if (size) *size = current_size;
        return read_block(current_extent);
    }
    
    char* token = search_path;
    while (*token) {
        char* next_slash = token;
        while (*next_slash && *next_slash != '/') next_slash++;
        
        char component[256];
        size_t comp_len = next_slash - token;
        if (comp_len >= sizeof(component)) {
            if (size) *size = 0;
            return NULL;
        }
        memcpy(component, token, comp_len);
        component[comp_len] = '\0';
        
        iso9660_dir_entry_t* entry = find_entry_in_dir(current_extent, current_size, component);
        if (!entry) {
            if (size) *size = 0;
            return NULL;
        }
        
        current_extent = entry->extent_le;
        current_size = entry->size_le;
        
        token = next_slash;
        if (*token == '/') token++;
    }
    
    if (size) *size = current_size;
    return read_block(current_extent);
}

void iso9660_list_dir(const char* path) {
    if (!initialized || !primary_volume) {
        kprint("ISO9660 not initialized\n", 14);
        return;
    }
    
    size_t dir_size;
    const void* dir_data = iso9660_find_file(path, &dir_size);
    
    if (!dir_data) {
        kprint("Directory not found: ", 14);
        kprint(path, 14);
        kprint("\n", 14);
        return;
    }
    
    kprint("Contents of ", 7);
    kprint(path, 7);
    kprint(":\n", 7);
    
    size_t offset = 0;
    while (offset < dir_size) {
        iso9660_dir_entry_t* entry = (iso9660_dir_entry_t*)((uint8_t*)dir_data + offset);
        if (entry->length == 0) break;
        
        char* entry_name = (char*)(entry + 1);
        uint8_t name_len = entry->name_len;
        
        if (name_len == 1 && (entry_name[0] == 0 || entry_name[0] == 1)) {
            offset += entry->length;
            continue;
        }
        
        char normalized[256];
        normalize_filename(entry_name, name_len, normalized, sizeof(normalized));
        
        kprint("  ", 7);
        if (entry->flags & ISO_FLAG_DIRECTORY) {
            kprint("[DIR]  ", 11);
        } else {
            kprint("[FILE] ", 7);
        }
        kprint(normalized, 11);
        
        if (!(entry->flags & ISO_FLAG_DIRECTORY)) {
            kprint(" (", 7);
            char buf[32];
            itoa(entry->size_le, buf, 10);
            kprint(buf, 7);
            kprint(" bytes)", 7);
        }
        kprint("\n", 7);
        
        offset += entry->length;
    }
}

bool iso9660_is_initialized(void) {
    return initialized;
}

static void mount_dir_recursive(const char* mount_point, const char* iso_path, uint32_t dir_extent, uint32_t dir_size);

void iso9660_mount_to_vfs(const char* mount_point, const char* iso_path) {
    if (!initialized || !primary_volume) {
        return;
    }
    
    extern void vfs_create(const char* filename, const char* data, size_t size);
    extern void vfs_mkdir(const char* dirname);
    
    size_t dir_size;
    const void* dir_data = iso9660_find_file(iso_path, &dir_size);
    if (!dir_data) return;
    
    char lower_mount_point[256];
    size_t mp_len = strlen(mount_point);
    if (mp_len >= sizeof(lower_mount_point)) return;
    memcpy(lower_mount_point, mount_point, mp_len + 1);
    to_lowercase(lower_mount_point);
    vfs_mkdir(lower_mount_point);
    
    size_t offset = 0;
    while (offset < dir_size) {
        iso9660_dir_entry_t* entry = (iso9660_dir_entry_t*)((uint8_t*)dir_data + offset);
        if (entry->length == 0) break;
        
        char* entry_name = (char*)(entry + 1);
        uint8_t name_len = entry->name_len;
        
        if (name_len == 1 && (entry_name[0] == 0 || entry_name[0] == 1)) {
            offset += entry->length;
            continue;
        }
        
        char normalized[256];
        normalize_filename(entry_name, name_len, normalized, sizeof(normalized));
        to_lowercase(normalized);
        
        char vfs_path[512];
        int idx = 0;

        for (int i = 0; lower_mount_point[i] && idx < 510; i++) {
            vfs_path[idx++] = lower_mount_point[i];
        }
        if (vfs_path[idx-1] != '/') {
            vfs_path[idx++] = '/';
        }

        for (int i = 0; normalized[i] && idx < 511; i++) {
            vfs_path[idx++] = normalized[i];
        }
        vfs_path[idx] = '\0';
        
        if (entry->flags & ISO_FLAG_DIRECTORY) {
            vfs_mkdir(vfs_path);
            mount_dir_recursive(vfs_path, iso_path, entry->extent_le, entry->size_le);
        } else {
            void* file_data = read_block(entry->extent_le);
            if (file_data && entry->size_le <= 4096) {
                vfs_create(vfs_path, (const char*)file_data, entry->size_le);
            }
        }
        
        offset += entry->length;
    }
}

static void mount_dir_recursive(const char* mount_point, const char* iso_path, uint32_t dir_extent, uint32_t dir_size) {
    extern void vfs_create(const char* filename, const char* data, size_t size);
    extern void vfs_mkdir(const char* dirname);
    
    uint8_t* dir_data = (uint8_t*)read_block(dir_extent);
    if (!dir_data) return;
    
    size_t offset = 0;
    while (offset < dir_size) {
        iso9660_dir_entry_t* entry = (iso9660_dir_entry_t*)(dir_data + offset);
        
        if (entry->length == 0) break;
        
        char* entry_name = (char*)(entry + 1);
        uint8_t name_len = entry->name_len;
        
        if (name_len == 1 && (entry_name[0] == 0 || entry_name[0] == 1)) {
            offset += entry->length;
            continue;
        }
        
        char normalized[256];
        normalize_filename(entry_name, name_len, normalized, sizeof(normalized));
        to_lowercase(normalized);
        
        char vfs_path[512];
        int idx = 0;

        for (int i = 0; mount_point[i] && idx < 510; i++) {
            vfs_path[idx++] = mount_point[i];
        }
        if (vfs_path[idx-1] != '/') {
            vfs_path[idx++] = '/';
        }

        for (int i = 0; normalized[i] && idx < 511; i++) {
            vfs_path[idx++] = normalized[i];
        }
        vfs_path[idx] = '\0';
        
        if (entry->flags & ISO_FLAG_DIRECTORY) {
            vfs_mkdir(vfs_path);
            mount_dir_recursive(vfs_path, iso_path, entry->extent_le, entry->size_le);
        } else {
            void* file_data = read_block(entry->extent_le);
            if (file_data && entry->size_le <= 4096) {
                vfs_create(vfs_path, (const char*)file_data, entry->size_le);
            }
        }
        
        offset += entry->length;
    }
}