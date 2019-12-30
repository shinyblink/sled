// A digital clock.
//
// Copyright (c) 2019, Adrian "vifino" Pistol <vifino@tty.sh>
// 
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "text.h"
#include "foxel35.xbm"
#include "xbm_font_loader.c"

#include <sys/types.h>
#include <unistd.h>

const int font_width = 4;
const int font_height = 6;

//typedef unsigned int[5][3] char35;

// notfound = {{1,1,1},
// 			  {1,0,1},
// 			  {1,0,1},
// 			  {1,0,1},
// 			  {1,1,1}};
//int notfound = 0b111
// unsigned int notfound[5][3]= {{1,1,1},
//  			  {1,0,1},
//  			  {1,0,1},
//  			  {1,0,1},
//  			  {1,1,1}};
//#define unknownCharacter {{1,1,1},{1,0,1},{1,0,1},{1,0,1},{1,1,1}}



#define FRAMETIME (T_SECOND)
#define FRAMES (TIME_SHORT)
#define CHARS_FULL 8 // 20:15:09, must also hold 20:15 (small)

struct font_char{
	char c;
	RGB fg;
	RGB bg;
};

static oscore_time nexttick;
static int frame = 0;
static int moduleno;
static int usesmall;
static char clockstr[CHARS_FULL + 1];
static text* rendered = NULL;
static int max_row;
static int max_column;
struct font_char* buffer;
char* type_buffer[] = {"echo mom | tr m l","fortune | cowsay","ip a"};//first is last
int type_pos;
int type_index;
int max_index;
RGB fg = RGB(247,127,190);
RGB bg = RGB(50,50,50);
int current_row = 0;

//scroll buffer up by one line
static void scroll_up(){
	int i;
	for(i = 0; i < ((max_column * (max_row - 1)));++i){
		buffer[i].c = buffer[i + max_column].c;
		buffer[i].fg = buffer[i + max_column].fg;
		buffer[i].bg = buffer[i + max_column].bg;
	}
	for(;i < ((max_column * max_row));++i){
		buffer[i].c = ' ';
		buffer[i].fg = RGB(255,255,255);
		buffer[i].bg = RGB(0,0,0);
	}
}

// returns the row on which the output ends
static int write_buffer(char* str, int row, int column, RGB fg, RGB bg){
	int i;
	int pos = column + (row * max_column);
	int len = strlen(str);
	//check whether it does fit
	while(pos + len > max_row * max_column){
		scroll_up();
		row--;
		pos = column + (row * max_column);
	}
	for(i = 0; i < len; ++i){
		if(str[i] != '\n'){
			//printf("- %d %d\n", pos, i);
			//printf("%c %c\n", buffer[pos].c, str[i]);
			if(pos >= 0){
				buffer[pos].c = str[i];
				buffer[pos].fg = fg;
				buffer[pos].bg = bg;
			}
			pos++;
		}else{//\n
			pos = ((pos/max_column) + 1)*max_column;
		}
	}
	// calculate the last row of our output
	while(len > max_column){
		len -= max_column;
		row++;
	};
	return row;
}

static void launch(char* command){
	char cmd[max_column*3];
	//sprintf(cmd, "$ %s\n", command);
	//current_row = write_buffer(cmd, current_row, 0, fg, bg);
	//current_row++;
	FILE *fp;
	//int mypid = fork();
	//char* args[] = {"-c",command};
	//if(mypid == 0){
		//execv("/bin/sh", args);
		//execl("/bin/sh", "-c", command, (char*)0);
	//}
	FILE* cout = popen(command, "r");
	char tmpbuffer[max_column*3];
	while(fgets(tmpbuffer, max_column*3,cout)!=NULL){
		current_row = write_buffer(tmpbuffer, current_row, 0, fg, bg);
		current_row++;
	};
	fclose(cout);
	//printf("wat: %s", tmpbuffer);
}

static void clear_buffer(){
	int i;
	for(i=0; i< max_row * max_column; ++i){
		buffer[i].c=' ';
		buffer[i].fg=RGB(255,255,255);
		buffer[i].bg=RGB(0,0,0);
	}
}

int init (int modno, char* argstr) {
	moduleno = modno;
	
	//for(i = 0; i < 32; ++i)
	//	foxel35[i] = notfound;

	max_row = matrix_gety() / 6;
	max_column = matrix_getx() / 4;

	max_index = 2;
	type_pos = 0;

	buffer = malloc(max_row * max_column * sizeof(struct font_char));
	//type_buffer = malloc(3 * sizeof(char*));


	//type_buffer[0] = "echo mom | tr m l";
	//type_buffer[1] = "echo mom | tr m l";
	//type_buffer[2] = "ip a";
	

	clear_buffer();
	write_buffer("Hello 36C3!", 2, 3, RGB(247, 127, 190), RGB(50,50,50));
	write_buffer("Hello 36C3!", 3, 10, RGB(255, 0, 0), RGB(50,50,50));
	write_buffer("Hello 36C3!", 4, 17, RGB(0, 255, 0), RGB(50,50,50));
	write_buffer("Hello 36C3!", 5, 24, RGB(0, 0, 255), RGB(50,50,50));
	write_buffer("Hello 36C3!", 6, 4, RGB(247, 127, 190), RGB(100,100,100));
	write_buffer("Hello 36C3!", 7, 11, RGB(255, 0, 0), RGB(100,100,100));
	write_buffer("Hello 36C3!", 8, 18, RGB(0, 255, 0), RGB(100,100,100));
	write_buffer("Hello 36C3!", 9, 25, RGB(0, 0, 255), RGB(100,100,100));
	write_buffer("Hello 36C3!", 10, 5, RGB(247, 127, 190), RGB(150,150,150));
	write_buffer("Hello 36C3!", 11, 12, RGB(255, 0, 0), RGB(150,150,150));
	write_buffer("Hello 36C3!", 12, 19, RGB(0, 255, 0), RGB(150,150,150));
	write_buffer("Hello 36C3!", 13, 26, RGB(0, 0, 255), RGB(150,150,150));
	RGB fg = RGB(247,127,190);
	RGB bg = RGB(50,50,50);
	scroll_up();
	scroll_up();

	//write_buffer("Line Break", 41, 60, RGB(247, 127, 190), RGB(50,50,50));
	//launch("echo mom | tr m l");
	//launch("echo mom | tr m l");
	//launch("ip a");
	//launch("fortune|cowsay");
	if(type_index >= 0){
		current_row = write_buffer("$ ", current_row, 0, fg, bg);
	}


	//2 rows
	if (matrix_getx() < 11)
		return 1; // not enough X to be usable
	// 8 characters
	if (matrix_gety() < 16)
		return 1; // not enough Y to be usable
	// max digit size is 4, plus 3 for :, so 4 + 4 + 3 + 4 + 4 + 3 + 4 + 4 = 24 + 6 = 30
	usesmall = matrix_getx() < 30;
	return 0;
}

void reset(int _modno) {
	type_index = max_index;
	type_pos = 0;
	current_row = 0;
	clear_buffer();
	if(type_index >= 0){
		current_row = write_buffer("$ ", current_row, 0, fg, bg);
	}
	nexttick = udate();
	frame = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	time_t rawtime;
	struct tm * timeinfo;
	const char * format = "%T";
	//unsigned char ch = 0;
	int x = 0;
	int y = 0;
	int row = 0;
	int column = 0;
	int pos = 0;
	char ch[1];
	if(type_index >=0 ){
		if(type_pos < strlen(type_buffer[type_index])){
			ch[0] = type_buffer[type_index][type_pos];
			current_row = write_buffer(ch, current_row, type_pos + 2, fg, bg);
			type_pos++;
		}else{
			current_row++;
			if(current_row > max_row){
				current_row--;
				scroll_up();
			}
			launch(type_buffer[type_index]);
			type_index--;
			type_pos = 0;
			if(type_index >= 0){
				current_row = write_buffer("$ ", current_row, 0, fg, bg);
			}else{
				return 1;
			}
		}
	}

	for(row = 0; row < max_row; ++row)
		for(column = 0; column < max_column; ++column){
			for (y = 0; y < 6; ++y){
				for (x = 0; x < 4; ++x) {
					matrix_set((column*4) + x,(row*6) + y, (load_char(foxel35_bits, buffer[pos].c,x,y,font_width,font_height) == 1?buffer[pos].fg:buffer[pos].bg));
				}
			}
			pos++;
		}

	matrix_render();
	frame++;
	nexttick += FRAMETIME;
	timer_add(nexttick, moduleno, 0, NULL);
	return 0;
}

void deinit(int _modno) {
	// This acts conditionally on rendered being non-NULL.
	text_free(rendered);
}



