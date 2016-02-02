package com.android.accessorydisplay.source;

import java.nio.ByteBuffer;
import android.util.Log;

public final class ScreenRecord {
    private static native final void native_init();
    static {
        System.loadLibrary("screenrecord_jni");
        native_init();
    }
    /**
     * startup native layer
     * @param args
     * @return native context
     */
    public native long native_startup(String args);

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
        public void notify_update(int update_flag, int arg1, int arg2);
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
    private void notify_update(int update_flag, int arg1, int arg2) {
        if (mNativeEventListener != null) {
            mNativeEventListener.notify_update(update_flag, arg1, arg2);
        }
    }

}
