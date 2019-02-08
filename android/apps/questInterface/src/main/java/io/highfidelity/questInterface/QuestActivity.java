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

import android.content.Intent;
import android.os.Bundle;
import android.view.WindowManager;

import io.highfidelity.oculus.OculusMobileActivity;
import io.highfidelity.utils.HifiUtils;

public class QuestActivity extends OculusMobileActivity {
    private native void questNativeOnCreate();
    private native void questNativeOnDestroy();
    private native void questNativeOnPause();
    private native void questNativeOnResume();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        HifiUtils.upackAssets(getAssets(), getCacheDir().getAbsolutePath());
        questNativeOnCreate();
    }


    @Override
    protected void onPause() {
        super.onPause();
        questNativeOnPause();

    }

    @Override
    protected void onResume() {
        super.onResume();
        questNativeOnResume();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        questNativeOnDestroy();
    }

}
