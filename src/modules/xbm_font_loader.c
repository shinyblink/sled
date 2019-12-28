// c is the character you want to load
// x and y are pixel inside the character
// xbm must have 16 characters per row
// xbm must contain 128 characters
int load_char(unsigned char bits[], char c, int x, int y, int width, int height){
    int offset_x = (c%16) * width;
    int offset_y = (c/16) * height;
    int total_x = offset_x + x;
    int total_y = offset_y + y;
    int pos = total_y * (width * 16) + total_x;
    //printf("(..., %d, %d, %d, %d, %d)\n", c, x, y, width, height);
    ///printf("%d %d %d %d - %d - %d %d - %d %d -=- %x %x\n", x, y, total_x, total_y, pos, pos/8, pos%8, offset_x, offset_y, bits[pos/8], ((bits[pos/8] >> ((pos%8))) & 1));
    return (((bits[pos/8]) >> ((pos%8))) & 1);
}