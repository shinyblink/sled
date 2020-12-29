// Color conversion, mostly.
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

#include "types.h"

RGB HSV2RGB(HSV hsv)
{
	RGB rgb;
	byte region, remainder, p, q, t;

	if (hsv.s == 0)
	{
		rgb = RGB(hsv.v, hsv.v, hsv.v);
		return rgb;
	}

	region = hsv.h / 43;
	remainder = (hsv.h - (region * 43)) * 6;

	p = (hsv.v * (255 - hsv.s)) >> 8;
	q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
	t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

	switch (region)
	{
		case 0:
			rgb = RGB(hsv.v, t, p);
			break;
		case 1:
			rgb = RGB(q, hsv.v, p);
			break;
		case 2:
			rgb = RGB(p, hsv.v, t);
			break;
		case 3:
			rgb = RGB(p, q, hsv.v);
			break;
		case 4:
			rgb = RGB(t, p, hsv.v);
			break;
		default:
			rgb = RGB(hsv.v, p, q);
			break;
	}

	return rgb;
}

HSV RGB2HSV(RGB rgb)
{
	HSV hsv;
	byte rgbMin, rgbMax;

	rgbMin = rgb.red < rgb.green ? (rgb.red < rgb.blue ? rgb.red : rgb.blue) : (rgb.green < rgb.blue ? rgb.green : rgb.blue);
	rgbMax = rgb.red > rgb.green ? (rgb.red > rgb.blue ? rgb.red : rgb.blue) : (rgb.green > rgb.blue ? rgb.green : rgb.blue);

	hsv.v = rgbMax;
	if (hsv.v == 0)
	{
		hsv.h = 0;
		hsv.s = 0;
		return hsv;
	}

	hsv.s = 255 * (int32_t) (rgbMax - rgbMin) / hsv.v;
	if (hsv.s == 0)
	{
		hsv.h = 0;
		return hsv;
	}

	if (rgbMax == rgb.red)
		hsv.h = 0 + 43 * (rgb.green - rgb.blue) / (rgbMax - rgbMin);
	else if (rgbMax == rgb.green)
		hsv.h = 85 + 43 * (rgb.blue - rgb.red) / (rgbMax - rgbMin);
	else
		hsv.h = 171 + 43 * (rgb.red - rgb.green) / (rgbMax - rgbMin);

	return hsv;
}

RGB RGBlerp(byte v, RGB rgbA, RGB rgbB) {
	RGB rgb;
	rgb.red = (byte) rgbA.red + ((((uint) rgbB.red - rgbA.red) * v) / 255);
	rgb.green = (byte) rgbA.green + ((((uint) rgbB.green - rgbA.green) * v) / 255);
	rgb.blue = (byte) rgbA.blue + ((((uint) rgbB.blue - rgbA.blue) * v) / 255);
	rgb.alpha = (byte) rgbA.alpha + ((((uint) rgbB.alpha - rgbA.alpha) * v) / 255);
	return rgb;
}
