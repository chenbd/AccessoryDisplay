package com.android.accessorydisplay.source;

import java.nio.ByteBuffer;
import android.util.Log;

public final class ScreenRecord {
    static {
        System.loadLibrary("screenrecord_jni");
    }
    /**
     * startup native layer
     * @param args
     * @return native context
     */
    public native long native_init(
            int gOutputFormat,           // data format for output
            int gWantInfoScreen,    // do we want initial info screen?
            int gWantFrameTime,     // do we want times on each frame?
            int gVideoWidth,        // default width+height
            int gVideoHeight,
            int gBitRate,     // 4Mbps
            int gTimeLimitSec,
            String fileName);

    /**
     * startup native layer
     * @param args
     * @return native context
     */
    public native int native_startup(long ctx);

    /**
     * shutdown native layer
     * @param ctx native context handle, return value from native_startup()
     * @return 0 on success and -1 on error
     */
    public native int native_shutdown(long ctx);


    /**
     * native evnet listener interface
     */
    public interface NativeEventListener {
        /**
         * called from native layer to notify info chage
         * @param update_flag flag to identify which info or infos need update, valid values are UPDATE_FLAG_*
         *          when app receives this callback, call specific interface accroding update_flag to get data
         * @return void
         */
        public int notify_update(int update_flag, int arg1, int arg2);
    }
    /**
     * set native event listener
     * @param listener
     */
    public void setNativeListener (NativeEventListener listener) {
        mNativeEventListener = listener;
    }
    private NativeEventListener mNativeEventListener;
    /**
     * called from native layer to notify info chage
     * @param update_flag flag to identify which info or infos need update, valid values are UPDATE_FLAG_*
     * @return void
     */
    private int notify_update(int update_flag, int arg1, int arg2) {
        if (mNativeEventListener != null) {
            return mNativeEventListener.notify_update(update_flag, arg1, arg2);
        }
        return -1;
    }
    public native int native_setBuf(long ctx, ByteBuffer pcmbuf);

    /**
     * lock buffer to be used
     * this function MUST be called in the callback function notify_update()
     * @param ctx native context handle, return value from native_startup()
     * @param pcmbuf if pcmbuf is not null, pcm data is copied to the pcmbuf
     * @return >0 means pcmbuf size in Byte on success, -1 on error
     */
    public native int native_lockBuf(long ctx, byte[] pcmbuf);
}
