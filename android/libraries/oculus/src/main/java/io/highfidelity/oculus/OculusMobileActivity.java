//
//  Created by Bradley Austin Davis on 2018/11/20
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
package io.highfidelity.oculus;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.widget.LinearLayout;


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
    private native static void nativeOnDestroy();
    private native static void nativeOnSurfaceChanged(Surface s);

    private native void questNativeOnCreate();
    private native void questNativeOnDestroy();
    private native void questNativeOnPause();
    private native void questNativeOnResume();
    private native void questOnAppAfterLoad();


    private SurfaceView mView;
    private SurfaceHolder mSurfaceHolder;

    boolean isLoading =false;

    public void onCreate(Bundle savedInstanceState) {
        isLoading=true;
        super.onCreate(savedInstanceState);
        HifiUtils.upackAssets(getAssets(), getCacheDir().getAbsolutePath());

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
        isLoading=false;

        //isLoading=false;
        runOnUiThread(() -> {
            setContentView(mView);
            questOnAppAfterLoad();
        });
    }

    @Override
    protected void onDestroy() {
        Log.w(TAG, "QQQ onDestroy");
        super.onDestroy();

        if (mSurfaceHolder != null) {
            nativeOnSurfaceChanged(null);
        }
        nativeOnDestroy();
        questNativeOnDestroy();
    }

    @Override
    protected void onResume() {
        Log.w(TAG, "QQQ onResume");
        super.onResume();

        questNativeOnResume();
        nativeOnResume();

    }

    @Override
    protected void onPause() {
        Log.w(TAG, "QQQ onPause");
        super.onPause();

        if (!isLoading) {
            questNativeOnPause();
            nativeOnPause();
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.w(TAG, "QQQ surfaceCreated");
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
        Log.w(TAG, "QQQ surfaceDestroyed");
        nativeOnSurfaceChanged(null);
        mSurfaceHolder = null;
    }
}