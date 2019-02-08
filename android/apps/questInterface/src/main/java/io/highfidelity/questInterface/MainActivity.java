//
//  MainActivity.java
//  android/app/src/main/java
//
//  Created by Stephen Birarda on 1/26/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

package io.highfidelity.questInterface;

import android.app.Activity;
import android.app.Application;
import android.content.Intent;
import  android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

import org.qtproject.qt5.android.bindings.QtActivity;

import java.util.Timer;
import java.util.TimerTask;

import io.highfidelity.utils.HifiUtils;

public class MainActivity extends QtActivity {
    private native void nativeOnCreate();
    private native void nativeOnDestroy();
    private native void nativeOnPause();
    private native void nativeOnResume();

    private boolean isLoading;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        HifiUtils.upackAssets(getAssets(), getCacheDir().getAbsolutePath());
        nativeOnCreate();
    }


    public void onAppLoadedComplete() {

        Log.w("QQQ", "Returning to quest activity");
        runOnUiThread(()->{
            startActivity(new Intent(MainActivity.this, QuestActivity.class));
            //moveTaskToBack(true);
        });
    }

    @Override
    protected void onPause() {
        super.onPause();

            nativeOnPause();

    }

    @Override
    protected void onResume() {
        super.onResume();
        nativeOnResume();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        nativeOnDestroy();
    }
}
