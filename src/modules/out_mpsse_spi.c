// Use the MPSSE engine of FTDI chips to talk SPI.
// Then uses SPI to throw RGB565 at some device.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <types.h>
#include <timers.h>
#include <stdint.h>
#include <colors.h>
#include <mpsse.h>

typedef struct matrix {
	short vid, pid;
	int index, iface;
	int linediv, linepad;

	int x, y, w, h;
	struct mpsse_context *context;
	RGB *buffer;
} matrix_t;

int num_drivers;
matrix_t *drivers = NULL;

int max_x, max_y;

#define MAX_DRIVERS 16

static char* cmdbuffer;

int init (int moduleno, char *argstr) {
	fprintf(stderr, "modno = %i\n", moduleno);
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
		drivers[i].linediv = 1;
		drivers[i].linepad = 0;
		drivers[i].x = -1;
		drivers[i].y = -1;
		drivers[i].w = -1;
		drivers[i].h = -1;
		drivers[i].index = -1;
		drivers[i].iface = -1;

		char *str = driver_strs[i];
		char *arg;
		while(str && (arg = strsep(&str, ",")))
		{
			char *key = NULL;
			char *value = NULL;

			key = strsep(&arg, "=");
			value = arg;
			if(!key || !value) break;

			if(strcmp(key, "linediv") == 0)
			{
				drivers[i].linediv = atoi(value);
			}
			else if(strcmp(key, "linepad") == 0)
			{
				drivers[i].linepad = atoi(value);
			}
			else if(strcmp(key, "x") == 0)
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

		if(drivers[i].linediv < 1)
		{
			fprintf(stderr, "error: Driver %i can't divide lines by %i!", i, drivers[i].linediv);
			init_error = true;
		}
		if(drivers[i].linepad < 0)
		{
			fprintf(stderr, "error: Driver %i can't pad lines by %i!", i, drivers[i].linepad);
			init_error = true;
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

		drivers[i].context = OpenIndex(drivers[i].vid, drivers[i].pid, SPI0, THIRTY_MHZ, MSB, drivers[i].iface, NULL, NULL, drivers[i].index);
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

		drivers[i].buffer = calloc(drivers[i].w * drivers[i].h * sizeof(RGB), 1);
	}

	cmdbuffer = calloc(max_x * sizeof(int16_t) + 1, 1);

	return 0;
}

int getx(int _modno) {
	return max_x;
}
int gety(int _modno) {
	return max_y;
}

int set(int _modno, int x, int y, RGB color) {
	// x,y to driver
	for(int i = 0; i < num_drivers; i++)
	{
		if(x >= drivers[i].x && x < drivers[i].x + drivers[i].w &&
		   y >= drivers[i].y && y < drivers[i].y + drivers[i].h)
		{
			size_t offset = (x - drivers[i].x) + (y - drivers[i].y) * drivers[i].w;
			drivers[i].buffer[offset] = color;
			//printf("(%x,%x,%x) %z\n", color.red, color.green, color.blue, offset);
			break;
		}
	}

	return 0;
}

RGB get(int _modno, int x, int y) {
	for(int i = 0; i < num_drivers; i++)
	{
		if(x >= drivers[i].x && x < drivers[i].x + drivers[i].w &&
		   y >= drivers[i].y && y < drivers[i].y + drivers[i].h)
		{
			size_t offset = (x - drivers[i].x) + (y - drivers[i].y) * drivers[i].w;
			return drivers[i].buffer[offset];
		}
	}

	return RGB(0, 0, 0); // meh
}

int clear(int _modno) {
	for(int i = 0; i < num_drivers; i++)
	{
		memset(drivers[i].buffer, 0, drivers[i].w * drivers[i].h * sizeof(RGB));
	}
	return 0;
}

int render(void) {
	for(int i = 0; i < num_drivers; i++)
	{
		for (int row = 0; row < (drivers[i].h * drivers[i].linediv); row++) {
			// Convert framebuffer to RGB565
			cmdbuffer[0] = 0x80;
			RGB* rowbuf = drivers[i].buffer + row * (drivers[i].w / drivers[i].linediv);
			for (int x = 0; x < (drivers[i].w / drivers[i].linediv); x++) {
				uint16_t converted = RGB2RGB565(rowbuf[x]);
				//printf("%x\t%x\n", rowbuf[x], converted);
				cmdbuffer[(x * 2) + 1] = converted & 0xFF;
				cmdbuffer[(x * 2) + 2] = (converted >> 8) & 0xFF;
			}
			// Transfer line
			Start(drivers[i].context);
			Write(drivers[i].context, cmdbuffer, 1 + drivers[i].w * 2 / drivers[i].linediv);
			Stop(drivers[i].context);

			// Commit line
			cmdbuffer[0] = 0x08;
			cmdbuffer[1] = row;
			Start(drivers[i].context);
			Write(drivers[i].context, cmdbuffer, 2 + drivers[i].linepad);
			Stop(drivers[i].context);

			/*
			char* returned = NULL;
			do {
				cmdbuffer[0] = 0x00;
				cmdbuffer[1] = 0x00;
				Start(drivers[i].context);
				free(returned);
				returned = Transfer(drivers[i].context, cmdbuffer, 2);
				Stop(drivers[i].context);
			} while (returned && (((returned[0] | returned[1]) & 0x01) != 0x01));
			free(returned);*/
		}
	}

	for (int i = 0; i < num_drivers; i++) {
		// Switch buffers.
		cmdbuffer[0] = 0x04;
		Start(drivers[i].context);
		Write(drivers[i].context, cmdbuffer, 2);
		Stop(drivers[i].context);
	}

	// wait for vsync
	for (int i = 0; i < num_drivers; i++) {
		// Switch buffers.
		char* returned = NULL;
		do {
			cmdbuffer[0] = 0x00;
			cmdbuffer[1] = 0x00;
			Start(drivers[i].context);
			free(returned);
			returned = Transfer(drivers[i].context, cmdbuffer, 2);
			Stop(drivers[i].context);
		} while (returned && (((returned[0] | returned[1]) & 0x02) != 0x02));
		free(returned);
	}

	return 0;
}

oscore_time wait_until(int _modno, oscore_time desired_usec) {
	// Hey, we can just delegate work to someone else. Yay!
	return timers_wait_until_core(desired_usec);
}

void wait_until_break(int _modno) {
	printf("Wait until break\n");
	timers_wait_until_break_core();
}

void deinit(int _modno) {
	for(int i = 0; i < num_drivers; i++)
	{
		Close(drivers[i].context);
		free(drivers[i].buffer);
	}
	free(drivers);
	num_drivers = 0;
	drivers = NULL;
}
