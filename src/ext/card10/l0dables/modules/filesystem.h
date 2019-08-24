#ifndef EPICARDIUM_MODULE_FILESYSTEM_INCLUDED
#define EPICARDIUM_MODULE_FILESYSTEM_INCLUDED

/* ---------- FAT fs ------------------------------------------------------ */

#include <stdbool.h>
#include "epicardium.h"

/**
 * module initialization - to be called once at startup before any FreeRTOS tasks
 * have been started
 *
 * calls fatfs_attach
 */
void fatfs_init(void);

/**
 * initialize and mount the FLASH storage
 */
int fatfs_attach(void);

/** close all opened FDs, sync and deinitialize FLASH layer */
void fatfs_detach(void);

#endif//EPICARDIUM_MODULE_FILESYSTEM_INCLUDED
