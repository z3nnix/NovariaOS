// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/drivers/cdrom.h>
#include <core/kernel/kstd.h>

static void* iso_memory = NULL;
static size_t iso_size = 0;

bool cdrom_init(void) {
    return true;
}

void cdrom_set_iso_data(void* data, size_t size) {
    iso_memory = data;
    iso_size = size;
}

void* cdrom_read_sectors(uint32_t lba, uint32_t count) {
    if (!iso_memory) {
        return NULL;
    }
    
    uint32_t offset = lba * 2048;
    if (offset >= iso_size) {
        return NULL;
    }
    
    return (void*)((uint8_t*)iso_memory + offset);
}

size_t cdrom_get_iso_size(void) {
    return iso_size;
}

void* cdrom_get_iso_data(void) {
    return iso_memory;
}
