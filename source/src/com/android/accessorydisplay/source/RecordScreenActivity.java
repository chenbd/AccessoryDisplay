/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.accessorydisplay.source;

import com.android.accessorydisplay.common.Logger;
import com.android.accessorydisplay.source.presentation.DemoPresentation;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.Display;
import android.widget.TextView;

public class RecordScreenActivity extends Activity {
    private static final String TAG = "RecordScreenActivity";

    private static final String ACTION_USB_ACCESSORY_PERMISSION =
            "com.android.accessorydisplay.source.ACTION_USB_ACCESSORY_PERMISSION";

    private static final String MANUFACTURER = "Android";
    private static final String MODEL = "Accessory Display";

    private UsbManager mUsbManager;
    
    private TextView mLogTextView;
    private Logger mLogger;

    private boolean mConnected;
    private UsbAccessory mAccessory;
    private UsbAccessoryStreamTransport mTransport;

    private DisplaySourceService mDisplaySourceService;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mUsbManager = (UsbManager)getSystemService(Context.USB_SERVICE);

        setContentView(R.layout.source_activity);

        mLogTextView = (TextView) findViewById(R.id.logTextView);
        mLogTextView.setMovementMethod(ScrollingMovementMethod.getInstance());
        mLogger = new TextLogger();

        mLogger.log("Waiting for accessory display sink to be attached to USB...");
        new VirtualDisplayThread(800, 480, 120).start();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    class TextLogger extends Logger {
        @Override
        public void log(final String message) {
            Log.d(TAG, message);

            mLogTextView.post(new Runnable() {
                @Override
                public void run() {
                    mLogTextView.append(message);
                    mLogTextView.append("\n");
                }
            });
        }
    }
            
    private final class VirtualDisplayThread extends Thread {
        private static final int TIMEOUT_USEC = 1000000;

        private final int mWidth;
        private final int mHeight;
        private final int mDensityDpi;

        private volatile boolean mQuitting;

        public VirtualDisplayThread(int width, int height, int densityDpi) {
            mWidth = width;
            mHeight = height;
            mDensityDpi = densityDpi;
        }

        @Override
        public void run() {
             ScreenRecord sr = new ScreenRecord();
             long hdl = sr.native_startup("aa.mp4");
        }

        public void quit() {
            mQuitting = true;
        }
    }
}
