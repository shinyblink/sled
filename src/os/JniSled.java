package sh.tty.sled;

import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;

public class JniSled implements Runnable {
    static {
        System.loadLibrary("sled");
    }

    // only set these before starting the thread!!!
    public static int MATRIX_X = 256;
    public static int MATRIX_Y = 256;

    private int[] currentBuf;

    public native void main();

    @Override
    public void run() {
        main();
    }

    // TODO: maybe make this trigger a callback optionally?
    //  could use that to let the user immediately render,
    //  "pushing" instead of polling
    //  this would let us have a variable frame rate dictated by sled.
    public void setCurrentBuffer(int[] buf) {
        currentBuf = buf;
    }

    public int[] getCurrentBuffer() {
        return currentBuf;
    }
}