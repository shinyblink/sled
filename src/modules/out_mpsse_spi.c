// First pass at something to send the matrix to sled-fpga-hub75,
//  mostly according to Vifino's design (pixel layout changed)
// "Here goes nothing", basically. - 20kdc

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <types.h>
#include <timers.h>
#include <mpsse.h>

typedef struct matrix {
	short vid, pid;
	int index, iface;

	int x, y, w, h;
	struct mpsse_context *context;
	RGB *buffer;
} matrix_t;

int num_drivers;
matrix_t *drivers = NULL;

int max_x, max_y;

#define MAX_DRIVERS 16

int init(int modno, char *argstr) {
	fprintf(stderr, "modno = %i\n", modno);
	printf("memes! %s\n", argstr);

	if(!argstr)
	{
		printf("No arguments, wtf?");
		return 1;
	}

	char **driver_strs = malloc(MAX_DRIVERS * sizeof(char*));
	char *tok;

	while(argstr && (tok = strsep(&argstr, "/")))
	{
		driver_strs[num_drivers++] = tok;
	}

	bool init_error = false;
	drivers = malloc(sizeof(matrix_t) * num_drivers);

	for(int i = 0; i < num_drivers; i++)
	{
		drivers[i].vid = 0xffff;
		drivers[i].pid = 0xffff;
		drivers[i].x = -1;
		drivers[i].y = -1;
		drivers[i].w = -1;
		drivers[i].h = -1;

		char *str = driver_strs[i];
		int state = 0;
		char *arg;
		while(str && (arg = strsep(&str, ",")))
		{
			char *key = NULL;
			char *value = NULL;

			key = strsep(&arg, "=");
			value = arg;
			if(!key || !value) break;
			
			if(strcmp(key, "x") == 0)
			{
				drivers[i].x = atoi(value);
			}
			else if(strcmp(key, "y") == 0)
			{
				drivers[i].y = atoi(value);
			}
			else if(strcmp(key, "w") == 0)
			{
				drivers[i].w = atoi(value);
			}
			else if(strcmp(key, "h") == 0)
			{
				drivers[i].h = atoi(value);
			}
			else if(strcmp(key, "vid") == 0)
			{
				drivers[i].vid = strtoul(value, NULL, 16);
			}
			else if(strcmp(key, "pid") == 0)
			{
				drivers[i].pid = strtoul(value, NULL, 16);
			}
			else if(strcmp(key, "index") == 0)
			{
				drivers[i].index = atoi(value);
			}
			else if(strcmp(key, "iface") == 0)
			{
				switch(value[0])
				{
					case 'a':
						drivers[i].iface = IFACE_A;
					break;
					case 'b':
						drivers[i].iface = IFACE_B;
					break;
					case 'c':
						drivers[i].iface = IFACE_C;
					break;
					case 'd':
						drivers[i].iface = IFACE_D;
					break;
				}
			}
			else
			{
				fprintf(stderr, "error: Unknown option %s!\n", key);
			}
		}

		if(drivers[i].x == -1)
		{
			fprintf(stderr, "error: Driver %i is missing x!\n", i);
			init_error = true;
		}
		if(drivers[i].y == -1)
		{
			fprintf(stderr, "error: Driver %i is missing y!\n", i);
			init_error = true;
		}
		if(drivers[i].w == -1)
		{
			if(drivers[0].w != -1)
			{
				drivers[i].w = drivers[0].w;
			}
			else
			{
				fprintf(stderr, "Driver %i is missing width!\n", i);
				init_error = true;
			}
		}
		if(drivers[i].h == -1)
		{
			if(drivers[0].h != -1)
			{
				drivers[i].h = drivers[0].h;
			}
			else
			{
				fprintf(stderr, "Driver %i is missing height!\n", i);
				init_error = true;
			}
		}
		if(drivers[i].vid == -1)
		{
			// todo: real vidpid of icebreaker
			drivers[i].vid = 0x0403;
		}
		if(drivers[i].pid == -1)
		{
			drivers[i].pid = 0x6010;
		}

		if(drivers[i].index == -1)
		{
			drivers[i].index = 0;
		}

		if(drivers[i].iface == -1)
		{
			drivers[i].iface = IFACE_A;
		}

		drivers[i].context = OpenIndex(drivers[i].vid, drivers[i].pid, SPI0, THIRTY_MHZ, LSB, drivers[i].iface, NULL, NULL, drivers[i].index);
		if(!drivers[i].context->open)
		{
			fprintf(stderr, "error: Driver %i failed to open!\n", i);
			init_error = true;
		}
	}
	if(init_error) return -1;

	max_x = 0;
	max_y = 0;

	for(int i = 0; i < num_drivers; i++)
	{
		if(drivers[i].x + drivers[i].w > max_x) max_x = drivers[i].x + drivers[i].w;
		if(drivers[i].y + drivers[i].h > max_y) max_y = drivers[i].y + drivers[i].h;

		drivers[i].buffer = malloc(drivers[i].w * drivers[i].h * sizeof(RGB));
	}

	return 0;
}

int getx(void) {
	return max_x;
}
int gety(void) {
	return max_y;
}

int set(int x, int y, RGB color) {
	if (x < 0 || y < 0)
		return 1;
	if (x >= max_x || y >= max_y)
		return 2;

	// x,y to driver
	for(int i = 0; i < num_drivers; i++)
	{
		if(x >= drivers[i].x && x > drivers[i].x + drivers[i].w &&
		   y >= drivers[i].y && y > drivers[i].y + drivers[i].h)
		{
			drivers[i].buffer[y + x * drivers[i].h] = color;
			break;
		}
	}

	return 0;
}

RGB get(int x, int y) {
	// meh
	return RGB(0, 0, 0);
}

int clear(void) {
	int i = 0;
	for(int i = 0; i < num_drivers; i++)
	{
		memset(drivers[i].buffer, 0, drivers[i].w * drivers[i].h);
	}
	return 0;
}

int render(void) {
	for(int i = 0; i < num_drivers; i++)
	{
		Start(drivers[i].context);
		Write(drivers[i].context, (char*)drivers[i].buffer, drivers[i].w * drivers[i].h * sizeof(RGB));
		Stop(drivers[i].context);
	}
	return 0;
}

ulong wait_until(ulong desired_usec) {
	// Hey, we can just delegate work to someone else. Yay!
	return wait_until_core(desired_usec);
}

void wait_until_break(void) {
	printf("Wait until break\n");
	wait_until_break_core();
}

int deinit(void) {
	for(int i = 0; i < num_drivers; i++)
	{
		Close(drivers[i].context);
		free(drivers[i].buffer);
	}
	free(drivers);
	num_drivers = 0;
	drivers = NULL;

	return 0;
}
