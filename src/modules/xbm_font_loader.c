// c is the character you want to load
// x and y are pixel inside the character
// xbm must have 16 characters per row
// xbm must contain 128 characters
int load_char(unsigned char bits[], char c, int x, int y, int w, int h) {
    int offset_x = (c % 16) * w;
    int offset_y = (c / 16) * h;
    int total_x = offset_x + x;
    int total_y = offset_y + y;
    int pos = total_y * (w * 16) + total_x;
    return (((bits[pos / 8]) >> ((pos % 8))) & 1);
}