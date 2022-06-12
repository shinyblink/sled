// 5A-75E output.
//
// Copyright (c) 2019, Draradech <kasten.m@gmx.de>
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

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <stdint.h>

#include <types.h>
#include <timers.h>
#include <assert.h>

#define WIDTH 256
#define HEIGHT 256

#define xstr(x) str(x)
#define str(x) #x

#ifndef INTERFACE
#error "define INTERFACE in sledconf"
#endif

typedef struct
{
  uint8_t b;
  uint8_t g;
  uint8_t r;
} BGR;

BGR frame[HEIGHT][WIDTH];
int sockfd;
struct sockaddr_ll socket_address;

uint8_t bufferswap[] = \
"\x11\x22\x33\x44\x55\x66\x22\x22\x33\x44\x55\x66\x01\x07\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\xff\x05\x00\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

uint8_t line[] = \
"\x11\x22\x33\x44\x55\x66\x22\x22\x33\x44\x55\x66\x55\x00\x01\x00" \
"\x00\x01\x00\x08\x88\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0" \
"\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\x6f\x5f\x53\x6b" \
"\x52\x3e\x6e\x54\x3f\x73\x58\x42\x78\x5c\x45\x7d\x60\x49\x7e\x60" \
"\x49\x7d\x60\x47\x7d\x5f\x47\x7f\x61\x49\x83\x64\x4c\x83\x65\x4c" \
"\x81\x62\x49\x7f\x62\x48\x86\x68\x4e\x85\x68\x4d\x83\x66\x4b\x81" \
"\x65\x49\x82\x65\x49";

oscore_time wait_until(int _modno, oscore_time desired_usec)
{
  #ifdef CIMODE
  return desired_usec;
  #else
  return timers_wait_until_core(desired_usec);
  #endif
}

void wait_until_break(int _modno)
{
  #ifndef CIMODE
  timers_wait_until_break_core();
  #endif
}

int init(void) {
  struct ifreq if_idx;

  /* Open RAW socket to send on */
  if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) < 0)
  {
    perror("socket");
  }

  /* Get the index of the interface to send on */
  memset(&if_idx, 0, sizeof(struct ifreq));
  strncpy(if_idx.ifr_name, xstr(INTERFACE), IFNAMSIZ-1);
  if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
  {
    perror("SIOCGIFINDEX");
  }

  /* Index of the network device */
  socket_address.sll_ifindex = if_idx.ifr_ifindex;
  /* Address length*/
  socket_address.sll_halen = ETH_ALEN;
  /* Destination MAC */
  socket_address.sll_addr[0] = 0x11;
  socket_address.sll_addr[1] = 0x22;
  socket_address.sll_addr[2] = 0x33;
  socket_address.sll_addr[3] = 0x44;
  socket_address.sll_addr[4] = 0x55;
  socket_address.sll_addr[5] = 0x66;

  return 0;
}

int getx(int _modno)
{
  return WIDTH;
}

int gety(int _modno)
{
  return HEIGHT;
}

int set(int _modno, int x, int y, RGB color)
{
  frame[y][x].r = color.red;
  frame[y][x].g = color.green;
  frame[y][x].b = color.blue;
  return 0;
}

RGB get(int _modno, int x, int y)
{
  return RGB(frame[y][x].r, frame[y][x].g, frame[y][x].b);
}

int clear(int _modno)
{
  memset(frame, 0, HEIGHT * WIDTH * 3);
  return 0;
};

int render(void)
{
  // Limit frame rate. In this configuration, the matrix is only stable up to ~70fps
  static oscore_time last = 0;
  oscore_time now = udate();
  oscore_time elapsed = now - last;
  if (last != 0 && elapsed < 15000)
  {
    wait_until(1337, last + 15000);
    last += 15000;
  }
  else
  {
    last = now;
  }

  for (int i = 0; i < HEIGHT; ++i)
  {
    line[14] = i;
    memcpy(&line[21], &frame[i][0], WIDTH * 3);
    /* Send line packet */
    if (sendto(sockfd, line, sizeof(line)/sizeof(line[0]), 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
    {
      perror("Send line failed");
    }
  }

  usleep(2000);

  /* Send bufferswap packet */
  if (sendto(sockfd, bufferswap, sizeof(bufferswap)/sizeof(bufferswap[0]), 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
  {
    perror("Send bufferswap failed");
  }

  return 0;
}

void deinit(int _modno)
{
}

