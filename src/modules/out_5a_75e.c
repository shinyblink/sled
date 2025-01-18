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
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

#include <types.h>
#include <timers.h>
#include <assert.h>

#define WIDTH 256
#define HEIGHT 256

typedef struct
{
  uint8_t b;
  uint8_t g;
  uint8_t r;
} BGR;

RGB frame_in[HEIGHT][WIDTH];
BGR frame_out[HEIGHT][WIDTH];
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

void p_error(char* str)
{
  if(errno)
  {
    perror(str);
  }
  else
  {
    fputs(str, stderr);
    fputs("\n", stderr);
  }
  exit(-1);
}

int init(int moduleno, char *argstr) {
  struct ifreq if_idx;
  
  if (!argstr)
  {
    p_error("no network interface provided. use \"sled -o 5a_75e:ifname\" to specify interface");
  }

  /* Open RAW socket to send on */
  if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) < 0)
  {
    p_error("socket");
  }

  /* Get the index of the interface to send on */
  memset(&if_idx, 0, sizeof(struct ifreq));
  strncpy(if_idx.ifr_name, argstr, IFNAMSIZ-1);
  if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
  {
    p_error("SIOCGIFINDEX");
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
  frame_in[y][x] = color;
  return 0;
}

RGB get(int _modno, int x, int y)
{
  return frame_in[y][x];
}

int clear(int _modno)
{
  memset(frame_in, 0, HEIGHT * WIDTH * sizeof(RGB));
  for (int y = 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      frame_in[y][x].alpha = 255;
    }
  }
  return 0;
};

int render(void)
{
  time_t tm_now = time(NULL);
  struct tm *tm_struct = localtime(&tm_now);
  float hour = tm_struct->tm_hour + tm_struct->tm_min / 60.0;

  // analyse
  int pixsum = 0;
  for (int y = 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      pixsum += frame_in[y][x].red;
      pixsum += frame_in[y][x].green;
      pixsum += frame_in[y][x].blue;
    }
  }
  double bright = (double)pixsum / (WIDTH * HEIGHT);
  double fade = 127.0 / bright;
  fade = MIN(0.66, fade);
  // TODO: get location from commandline, implement equation of time for sunset and sunrise
  if (hour > 6 && hour < 20.5)
  {
    fade = 1.0;
  }
  
  // copy and fade
  for (int y = 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      frame_out[y][x].r = frame_in[y][x].red * fade;
      frame_out[y][x].g = frame_in[y][x].green * fade;
      frame_out[y][x].b = frame_in[y][x].blue * fade;
    }
  }
  
  // Limit frame rate. In this configuration, the matrix is only stable up to ~70fps
  static oscore_time last = 0;
  oscore_time now = udate();
  oscore_time elapsed = now - last;
  if (last != 0 && elapsed < 15000)
  {
    while(wait_until(1337, last + 15000) < last + 15000)
    {
      ;
    }
    last += 15000;
  }
  else
  {
    last = now;
  }

  for (int i = 0; i < HEIGHT; ++i)
  {
    line[14] = i;
    memcpy(&line[21], &frame_out[i][0], WIDTH * 3);
    /* Send line packet */
    if (sendto(sockfd, line, sizeof(line)/sizeof(line[0]), 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
    {
      p_error("Send line failed");
    }
  }

  usleep(2000);

  /* Send bufferswap packet */
  if (sendto(sockfd, bufferswap, sizeof(bufferswap)/sizeof(bufferswap[0]), 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
  {
    p_error("Send bufferswap failed");
  }

  return 0;
}

void deinit(int _modno)
{
}

