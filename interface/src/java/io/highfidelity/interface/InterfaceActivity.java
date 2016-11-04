//
//  InterfaceActivity.java
//  gvr-interface/java
//
//  Created by Stephen Birarda on 1/26/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

package io.highfidelity.gvrinterface;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.WindowManager;
import android.util.Log;
import org.qtproject.qt5.android.bindings.QtActivity;

public class InterfaceActivity extends QtActivity {
    
    public static native void handleHifiURL(String hifiURLString);
    
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        
        // Get the intent that started this activity in case we have a hifi:// URL to parse
        Intent intent = getIntent();
        if (intent.getAction() == Intent.ACTION_VIEW) {
            Uri data = intent.getData();
        
            if (data.getScheme().equals("hifi")) {
                handleHifiURL(data.toString());
            }
        }
        
    }
}