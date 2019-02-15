//
//  Created by Bradley Austin Davis on 2018/11/20
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
package io.highfidelity.frameplayer;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import org.qtproject.qt5.android.bindings.QtActivity;

import io.highfidelity.oculus.OculusMobileActivity;


public class QuestQtActivity extends QtActivity {
    private native void nativeOnCreate();
    private boolean launchedQuestMode = false;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.w("QQQ_Qt", "QuestQtActivity::onCreate");
        super.onCreate(savedInstanceState);
        nativeOnCreate();
    }

    @Override
    public void onDestroy() {
        Log.w("QQQ_Qt", "QuestQtActivity::onDestroy");
        super.onDestroy();
    }

    public void launchOculusActivity() {
        Log.w("QQQ_Qt", "QuestQtActivity::launchOculusActivity");
        runOnUiThread(()->{
            launchedQuestMode = true;
            moveTaskToBack(true);
            startActivity(new Intent(this, QuestRenderActivity.class));
        });
    }

    @Override
    public void onResume() {
        super.onResume();
        if (launchedQuestMode) {
            moveTaskToBack(true);
        }
    }
}
