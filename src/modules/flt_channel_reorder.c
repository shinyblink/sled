// Channel reorder.
// Switches channels around, if your matruc has different color order.
// Usage: -f channel_reorder:bgr
//
// Copyright (c) 2019, Draradech
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
#include <timers.h>
#include <mod.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static module* nextm;
static mod_flt* next;

static char chan[3];

int init(int nextno, char* argstr) {
	nextm = mod_get(nextno);
	next = nextm->mod;

	if (argstr && strlen(argstr) >= 3)
	{
	  chan[0] = argstr[0];
	  chan[1] = argstr[1];
	  chan[2] = argstr[2];
		free(argstr);
	}
	else
	{
	  chan[0] = 'r';
	  chan[1] = 'g';
	  chan[2] = 'b';
	}
	return 0;
}

int getx(void) {
	return next->getx();
}
int gety(void) {
	return next->gety();
}

int set(int x, int y, RGB cin) {
  RGB cout;
  cout.red   = chan[0] == 'g' ? cin.green : (chan[0] == 'b' ? cin.blue : cin.red);
  cout.green = chan[1] == 'g' ? cin.green : (chan[1] == 'b' ? cin.blue : cin.red);
  cout.blue  = chan[2] == 'g' ? cin.green : (chan[2] == 'b' ? cin.blue : cin.red);
  cout.alpha = cin.alpha;
	return next->set(x, y, cout);
}

RGB get(int x, int y) {
  RGB cin, cout;
	cout = next->get(x, y);
	cin.red   = chan[1] == 'r' ? cout.green : (chan[2] == 'r' ? cout.blue  : cout.red);
	cin.green = chan[0] == 'g' ? cout.red   : (chan[2] == 'g' ? cout.blue  : cout.green);
	cin.blue  = chan[0] == 'b' ? cout.red   : (chan[1] == 'b' ? cout.green : cout.blue);
	cin.alpha = cout.alpha;
	return cin;
}

int clear(void) {
	return next->clear();
}

int render(void) {
	return next->render();
}

ulong wait_until(ulong desired_usec) {
	return next->wait_until(desired_usec);
}

void wait_until_break(void) {
	if (next->wait_until_break)
		return next->wait_until_break();
}

int deinit(void) {
	return nextm->deinit(mod_getid(nextm));
}
