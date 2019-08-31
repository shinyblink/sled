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

typedef struct
{
  uint8_t b;
  uint8_t g;
  uint8_t r;
} RGB3;

RGB3 frame[128][128];
int sockfd;
struct sockaddr_ll socket_address;

uint8_t first[] = \
"\x11\x22\x33\x44\x55\x66\x22\x22\x33\x44\x55\x66\x01\x07\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\xff\x05\x00\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";


uint8_t second[] = \
"\x11\x22\x33\x44\x55\x66\x22\x22\x33\x44\x55\x66\x0a\xff\xff\xff" \
"\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";


uint8_t line[] = \
"\x11\x22\x33\x44\x55\x66\x22\x22\x33\x44\x55\x66\x55\x00\x00\x00" \
"\x00\x00\x80\x08\x88\x7f\x00\x00\x00\x7f\x00\x00\x00\x7f\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
"\x00\x00\x00\x00\x00";

int init(void) {
  struct ifreq if_idx;
  
  /* Open RAW socket to send on */
  if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1)
  {
    perror("socket");
  }

  /* Get the index of the interface to send on */
  memset(&if_idx, 0, sizeof(struct ifreq));
  strncpy(if_idx.ifr_name, "enp109s0", IFNAMSIZ-1);
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
	return 128;
}

int gety(int _modno)
{
	return 128;
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
  memset(frame, 0, 128 * 128 * 3);
	return 0;
};

int render(void)
{
  /* Send first packet */
  if (sendto(sockfd, first, sizeof(first)/sizeof(first[0]), 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
  {
    printf("Send first failed\n");
  }

  /* Send second packet */
  if (sendto(sockfd, second, sizeof(second)/sizeof(second[0]), 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
  {
    printf("Send second failed\n");
  }
  
  for (uint8_t i = 0; i < 128; ++i)
  {
    line[14] = i;
    memcpy(&line[21], &frame[i][0], 128 * 3);
    /* Send line packet */
    if (sendto(sockfd, line, sizeof(line)/sizeof(line[0]), 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
    {
      printf("Send line %d failed\n", i);
    }
  }
  
  #ifndef CIMODE
  //usleep(25000);
  #endif
	return 0;
}

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

void deinit(int _modno)
{
}

