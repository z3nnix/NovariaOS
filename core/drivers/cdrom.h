#ifndef CDROM_H
#define CDROM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Initialize CD-ROM driver and detect the boot ISO
bool cdrom_init(void);

// Read sectors from CD-ROM
// Returns pointer to data in memory (allocated statically)
void* cdrom_read_sectors(int32_t lba, int32_t count);

// Get the size of the boot ISO in bytes (if available)
size_t cdrom_get_iso_size(void);

// Get pointer to the ISO data in memory (after loading)
void* cdrom_get_iso_data(void);

// Set ISO data pointer (used during boot to point to the ISO in memory)
void cdrom_set_iso_data(void* data, size_t size);

#endif // CDROM_H
