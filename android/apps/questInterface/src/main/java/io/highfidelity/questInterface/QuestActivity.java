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

import io.highfidelity.oculus.OculusMobileActivity;

public class QuestActivity extends OculusMobileActivity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        startActivity(new Intent(this, MainActivity.class));
    }
}
