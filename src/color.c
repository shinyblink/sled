// Color conversion, mostly.

#include "types.h"

RGB HSV2RGB(HSV hsv)
{
    RGB rgb;
    byte region, remainder, p, q, t;

    if (hsv.s == 0)
    {
        rgb.red = hsv.v;
        rgb.green = hsv.v;
        rgb.blue = hsv.v;
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
            rgb.red = hsv.v;
						rgb.green = t;
						rgb.blue = p;
            break;
        case 1:
            rgb.red = q;
						rgb.green = hsv.v;
						rgb.blue = p;
            break;
        case 2:
            rgb.red = p;
						rgb.green = hsv.v;
						rgb.blue = t;
            break;
        case 3:
            rgb.red = p;
						rgb.green = q;
						rgb.blue = hsv.v;
            break;
        case 4:
            rgb.red = t;
						rgb.green = p;
						rgb.blue = hsv.v;
            break;
        default:
            rgb.red = hsv.v;
						rgb.green = p;
						rgb.blue = q;
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

    hsv.s = 255 * (long) (rgbMax - rgbMin) / hsv.v;
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
	return rgb;
}
