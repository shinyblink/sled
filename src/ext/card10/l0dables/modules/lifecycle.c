#include "epicardium.h"
#include "modules/log.h"
#include "modules/modules.h"
#include "api/dispatcher.h"
#include "api/interrupt-sender.h"
#include "l0der/l0der.h"

#include "card10.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <string.h>
#include <stdbool.h>
#include <stdbool.h>

#define PYCARDIUM_IVT (void *)0x10080000
#define BLOCK_WAIT pdMS_TO_TICKS(1000)
/*
 * Loading an empty filename into Pycardium will drop straight into the
 * interpreter.  This define is used to make it more clear when we intend
 * to go into the interpreter.
 */
#define PYINTERPRETER ""

static TaskHandle_t lifecycle_task = NULL;
static StaticSemaphore_t core1_mutex_data;
static SemaphoreHandle_t core1_mutex = NULL;

enum payload_type {
	PL_INVALID       = 0,
	PL_PYTHON_SCRIPT = 1,
	PL_PYTHON_DIR    = 2,
	PL_PYTHON_INTERP = 3,
	PL_L0DABLE       = 4,
};

struct load_info {
	bool do_reset;
	enum payload_type type;
	char name[256];
};
static volatile struct load_info async_load = {
	.do_reset = false,
	.name     = { 0 },
	.type     = PL_INVALID,
};

/* Whether to write the menu script before attempting to load. */
static volatile bool write_menu = false;

/* Helpers {{{ */

/*
 * Check if the payload is a valid file (or module) and if so, return its type.
 */
static int load_stat(char *name)
{
	size_t name_len = strlen(name);

	if (name_len == 0) {
		return PL_PYTHON_INTERP;
	}

	struct epic_stat stat;
	if (epic_file_stat(name, &stat) < 0) {
		return -ENOENT;
	}

	if (stat.type == EPICSTAT_DIR) {
		/* This might be a python module. */
		return PL_PYTHON_DIR;
	}

	if (strcmp(name + name_len - 3, ".py") == 0) {
		/* A python script */
		return PL_PYTHON_SCRIPT;
	} else if (strcmp(name + name_len - 4, ".elf") == 0) {
		return PL_L0DABLE;
	}

	return -ENOEXEC;
}

/*
 * Actually load a payload into core 1.  Optionally reset the core first.
 */
static int do_load(struct load_info *info)
{
	struct l0dable_info l0dable;
	int res;

	if (*info->name == '\0') {
		LOG_INFO("lifecycle", "Loading Python interpreter ...");
	} else {
		LOG_INFO("lifecycle", "Loading \"%s\" ...", info->name);
	}

	if (xSemaphoreTake(api_mutex, BLOCK_WAIT) != pdTRUE) {
		LOG_ERR("lifecycle", "API blocked");
		return -EBUSY;
	}

	if (info->do_reset) {
		LOG_DEBUG("lifecycle", "Triggering core 1 reset.");

		core1_trigger_reset();
	}

	/*
	 * Wait for the core to become ready to accept a new payload.
	 */
	core1_wait_ready();

	/*
	 * Reinitialize Hardware & Drivers
	 */
	res = hardware_reset();
	if (res < 0) {
		return res;
	}

	switch (info->type) {
	case PL_PYTHON_SCRIPT:
	case PL_PYTHON_DIR:
	case PL_PYTHON_INTERP:
		core1_load(PYCARDIUM_IVT, info->name);
		break;
	case PL_L0DABLE:
		res = l0der_load_path(info->name, &l0dable);
		if (res != 0) {
			LOG_ERR("lifecycle", "l0der failed: %d\n", res);
			xSemaphoreGive(api_mutex);
			return -ENOEXEC;
		}
		core1_load(l0dable.isr_vector, "");

		break;
	default:
		LOG_ERR("lifecyle",
			"Attempted to load invalid payload (%s)",
			info->name);
		xSemaphoreGive(api_mutex);
		return -EINVAL;
	}

	xSemaphoreGive(api_mutex);
	return 0;
}

/*
 * Do a synchroneous load.
 */
static int load_sync(char *name, bool reset)
{
	int ret = load_stat(name);
	if (ret < 0) {
		return ret;
	}

	struct load_info info = {
		.name     = { 0 },
		.type     = ret,
		.do_reset = reset,
	};

	strncpy(info.name, name, sizeof(info.name));

	return do_load(&info);
}

/*
 * Do an asynchroneous load.  This will return immediately if the payload seems
 * valid and call the lifecycle task to actually perform the load later.
 */
static int load_async(char *name, bool reset)
{
	int ret = load_stat(name);
	if (ret < 0) {
		return ret;
	}

	async_load.type     = ret;
	async_load.do_reset = reset;

	strncpy((char *)async_load.name, name, sizeof(async_load.name));

	if (lifecycle_task != NULL) {
		xTaskNotifyGive(lifecycle_task);
	}

	return 0;
}

/*
 * Epicardium contains an embedded default menu script which it writes to
 * external flash if none is found there.  This way, you won't make your card10
 * unusable by accidentally removing the menu script.
 *
 * You can find the sources for the menu-script in `preload/menu.py`.
 */

/*
 * Embed the menu.py script in the Epicardium binary.
 */
__asm(".section \".rodata\"\n"
      "_menu_script_start:\n"
      ".incbin \"../preload/menu.py\"\n"
      "_menu_script_end:\n"
      ".previous\n");

extern const uint8_t _menu_script_start;
extern const uint8_t _menu_script_end;

static int write_default_menu(void)
{
	const size_t length =
		(uintptr_t)&_menu_script_end - (uintptr_t)&_menu_script_start;
	int ret;

	LOG_INFO("lifecycle", "Writing default menu ...");

	int fd = epic_file_open("menu.py", "w");
	if (fd < 0) {
		return fd;
	}

	ret = epic_file_write(fd, &_menu_script_start, length);
	if (ret < 0) {
		return ret;
	}

	ret = epic_file_close(fd);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/*
 * Go back to the menu.
 */
static void load_menu(bool reset)
{
	LOG_DEBUG("lifecycle", "Into the menu");

	if (xSemaphoreTake(core1_mutex, BLOCK_WAIT) != pdTRUE) {
		LOG_ERR("lifecycle",
			"Can't load because mutex is blocked (menu).");
		return;
	}

	int ret = load_async("menu.py", reset);
	if (ret < 0) {
		LOG_WARN("lifecycle", "No menu script found.");

		/* The lifecycle task will perform the write */
		write_menu = true;

		async_load.type     = PL_PYTHON_SCRIPT;
		async_load.do_reset = reset;
		strncpy((char *)async_load.name,
			"menu.py",
			sizeof(async_load.name));

		if (lifecycle_task != NULL) {
			xTaskNotifyGive(lifecycle_task);
		}
	}

	xSemaphoreGive(core1_mutex);
}
/* Helpers }}} */

/* API {{{ */
/*
 * Restart the firmware
 */
void epic_system_reset(void)
{
	card10_reset();
}

/*
 * This is NOT the epic_exec() called from Pycardium, but an implementation of
 * the same call for use in Epicardium.  This function is synchroneous and will
 * wait until the call returns.
 */
int epic_exec(char *name)
{
	if (xSemaphoreTake(core1_mutex, BLOCK_WAIT) != pdTRUE) {
		LOG_ERR("lifecycle",
			"Can't load because mutex is blocked (epi exec).");
		return -EBUSY;
	}

	int ret = load_sync(name, true);
	xSemaphoreGive(core1_mutex);
	return ret;
}

/*
 * This is the underlying call for epic_exec() from Pycardium.  It is
 * asynchroneous and will return early to allow Pycardium (or a l0dable) to jump
 * to the reset handler.
 *
 * The lifecycle task will deal with actually loading the new payload.
 */
int __epic_exec(char *name)
{
	if (xSemaphoreTake(core1_mutex, BLOCK_WAIT) != pdTRUE) {
		LOG_ERR("lifecycle",
			"Can't load because mutex is blocked (1 exec).");
		return -EBUSY;
	}
	int ret = load_async(name, false);
	xSemaphoreGive(core1_mutex);
	return ret;
}

/*
 * This is the underlying call for epic_exit() from Pycardium.  It is
 * asynchroneous and will return early to allow Pycardium (or a l0dable) to jump
 * to the reset handler.
 *
 * The lifecycle task will deal with actually loading the new payload.
 */
void __epic_exit(int ret)
{
	if (ret == 0) {
		LOG_INFO("lifecycle", "Payload returned successfully");
	} else {
		LOG_WARN("lifecycle", "Payload returned with %d.", ret);
	}

	load_menu(false);
}

/*
 * This function can be used in Epicardium to jump back to the menu.
 *
 * It is asynchroneous and will return immediately.  The lifecycle task will
 * take care of actually jumping back.
 */
void return_to_menu(void)
{
	load_menu(true);
}
/* API }}} */

void vLifecycleTask(void *pvParameters)
{
	lifecycle_task = xTaskGetCurrentTaskHandle();
	core1_mutex    = xSemaphoreCreateMutexStatic(&core1_mutex_data);

	if (xSemaphoreTake(core1_mutex, 0) != pdTRUE) {
		LOG_CRIT(
			"lifecycle", "Failed to acquire mutex after creation."
		);
		vTaskDelay(portMAX_DELAY);
	}

	LOG_DEBUG("lifecycle", "Booting core 1 ...");
	core1_boot();
	vTaskDelay(pdMS_TO_TICKS(10));

	xSemaphoreGive(core1_mutex);

	/* If `main.py` exists, start it.  Otherwise, start `menu.py`. */
	if (epic_exec("main.py") < 0) {
		return_to_menu();
	}

	hardware_init();

	/* When triggered, reset core 1 to menu */
	while (1) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if (xSemaphoreTake(core1_mutex, BLOCK_WAIT) != pdTRUE) {
			LOG_ERR("lifecycle",
				"Can't load because mutex is blocked (task).");
			continue;
		}

		if (write_menu) {
			write_menu = false;
			int ret    = write_default_menu();
			if (ret < 0) {
				LOG_ERR("lifecycle",
					"Failed to write default menu: %d",
					ret);
				load_async(PYINTERPRETER, "");
				ulTaskNotifyTake(pdTRUE, 0);
			}
		}

		do_load((struct load_info *)&async_load);

		xSemaphoreGive(core1_mutex);
	}
}
