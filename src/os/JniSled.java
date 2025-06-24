package sh.tty.sled;

import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;

public class JniSled implements Runnable {
    static {
        System.loadLibrary("sled");
    }

    private int matrixX = 256;
    private int matrixY = 256;

    private int[] currentBuf;

    public JniSled(int x, int y) {
        matrixX = x;
        matrixY = y;
    }

    public native void main();

    @Override
    public void run() {
        main();
    }

    public int getMatrixX() {
        return matrixX;
    }

    public int getMatrixY() {
        return matrixY;
    }

    public int[] getCurrentBuffer() {
        return currentBuf;
    }

    // called from out_jnibuf
    // TODO: maybe make this trigger a callback optionally?
    //  could use that to let the user immediately render,
    //  "pushing" instead of polling
    //  this would let us have a variable frame rate dictated by sled.
    private void setCurrentBuffer(int[] buf) {
        currentBuf = buf;
    }
}