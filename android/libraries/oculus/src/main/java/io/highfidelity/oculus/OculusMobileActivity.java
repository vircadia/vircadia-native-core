//
//  Created by Bradley Austin Davis on 2018/11/20
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
package io.highfidelity.oculus;

import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import org.qtproject.qt5.android.bindings.QtActivity;
import io.highfidelity.utils.HifiUtils;

/**
 * Contains a native surface and forwards the activity lifecycle and surface lifecycle
 * events to the OculusMobileDisplayPlugin
 */
public class OculusMobileActivity extends QtActivity implements SurfaceHolder.Callback {
    private static final String TAG = OculusMobileActivity.class.getSimpleName();
    static { System.loadLibrary("oculusMobile"); }

    private native void nativeOnCreate();
    private native static void nativeOnResume();
    private native static void nativeOnPause();
    private native static void nativeOnSurfaceChanged(Surface s);

    private native void questNativeOnCreate();
    private native void questNativeOnPause();
    private native void questNativeOnResume();
    private native void questOnAppAfterLoad();


    private SurfaceView mView;
    private SurfaceHolder mSurfaceHolder;

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.w(TAG, "QQQ onCreate");
        // Create a native surface for VR rendering (Qt GL surfaces are not suitable
        // because of the lack of fine control over the surface callbacks)
        // Forward the create message to the JNI code
        mView = new SurfaceView(this);
        mView.getHolder().addCallback(this);

        nativeOnCreate();
        questNativeOnCreate();
    }
    public void onAppLoadedComplete() {
        Log.w(TAG, "QQQ Load Completed");
        runOnUiThread(() -> {
            setContentView(mView);
            questOnAppAfterLoad();
        });
    }

    @Override
    protected void onDestroy() {
        Log.w(TAG, "QQQ onDestroy");

        nativeOnSurfaceChanged(null);

        Log.w(TAG, "QQQ onDestroy -- SUPER onDestroy");
        super.onDestroy();
    }

    @Override
    protected void onResume() {
        Log.w(TAG, "QQQ onResume");
        super.onResume();
        //Reconnect the global reference back to handler
        nativeOnCreate();

        questNativeOnResume();
        nativeOnResume();
    }

    @Override
    protected void onPause() {
        Log.w(TAG, "QQQ onPause");
        super.onPause();

        questNativeOnPause();
        nativeOnPause();
    }

    @Override
    protected void onStop(){
        super.onStop();
        Log.w(TAG, "QQQ Onstop called");
    }

    @Override
    protected void onRestart(){
        super.onRestart();
        Log.w(TAG, "QQQ onRestart called ****");
        questOnAppAfterLoad();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.w(TAG, "QQQ surfaceCreated ************************************");
        nativeOnSurfaceChanged(holder.getSurface());
        mSurfaceHolder = holder;
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.w(TAG, "QQQ surfaceChanged");
        nativeOnSurfaceChanged(holder.getSurface());
        mSurfaceHolder = holder;
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.w(TAG, "QQQ surfaceDestroyed ***************************************************");
        nativeOnSurfaceChanged(null);
        mSurfaceHolder = null;

    }
}