//
//  InterfaceActivity.java
//  android/app/src/main/java
//
//  Created by Stephen Birarda on 1/26/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

package io.highfidelity.hifiinterface;

import android.content.Intent;
import android.content.res.AssetManager;
import android.net.Uri;
import android.os.Bundle;
import android.view.WindowManager;
import android.util.Log;
import org.qtproject.qt5.android.bindings.QtActivity;

/*import com.google.vr.cardboard.DisplaySynchronizer;
import com.google.vr.cardboard.DisplayUtils;
import com.google.vr.ndk.base.GvrApi;*/
import android.graphics.Point;
import android.content.res.Configuration;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.view.View;

public class InterfaceActivity extends QtActivity {

    public static final String DOMAIN_URL = "url";
    private static final String TAG = "Interface";

    //public static native void handleHifiURL(String hifiURLString);
    private native long nativeOnCreate(InterfaceActivity instance, AssetManager assetManager);
    //private native void nativeOnPause();
    //private native void nativeOnResume();
    private native void nativeOnDestroy();
    private native void nativeGotoUrl(String url);
    private native void nativeGoBackFromAndroidActivity();
    private native void nativeEnterBackground();
    private native void nativeEnterForeground();
    //private native void saveRealScreenSize(int width, int height);
    //private native void setAppVersion(String version);
    private native long nativeOnExitVr();

    private AssetManager assetManager;

    private static boolean inVrMode;
//    private GvrApi gvrApi;
    // Opaque native pointer to the Application C++ object.
    // This object is owned by the InterfaceActivity instance and passed to the native methods.
    //private long nativeGvrApi;
    
    public void enterVr() {
        //Log.d("[VR]", "Entering Vr mode (java)");
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
        inVrMode = true;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Intent intent = getIntent();
        if (intent.hasExtra(DOMAIN_URL) && !intent.getStringExtra(DOMAIN_URL).isEmpty()) {
            intent.putExtra("applicationArguments", "--url " + intent.getStringExtra(DOMAIN_URL));
        }
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        if (intent.getAction() == Intent.ACTION_VIEW) {
            Uri data = intent.getData();

//            if (data.getScheme().equals("hifi")) {
//                handleHifiURL(data.toString());
//            }
        }
        
/*        DisplaySynchronizer displaySynchronizer = new DisplaySynchronizer(this, DisplayUtils.getDefaultDisplay(this));
        gvrApi = new GvrApi(this, displaySynchronizer);
        */
//        Log.d("GVR", "gvrApi.toString(): " + gvrApi.toString());

        assetManager = getResources().getAssets();

        //nativeGvrApi =
            nativeOnCreate(this, assetManager /*, gvrApi.getNativeGvrContext()*/);

        Point size = new Point();
        getWindowManager().getDefaultDisplay().getRealSize(size);
//        saveRealScreenSize(size.x, size.y);

        try {
            PackageInfo pInfo = this.getPackageManager().getPackageInfo(getPackageName(), 0);
            String version = pInfo.versionName;
//            setAppVersion(version);
        } catch (PackageManager.NameNotFoundException e) {
            Log.e("GVR", "Error getting application version", e);
        }

        final View rootView = getWindow().getDecorView().findViewById(android.R.id.content);

        // This is a workaround to hide the menu bar when the virtual keyboard is shown from Qt
        rootView.getViewTreeObserver().addOnGlobalLayoutListener(new android.view.ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                int heightDiff = rootView.getRootView().getHeight() - rootView.getHeight();
                if (getActionBar().isShowing()) {
                    getActionBar().hide();
                }
            }
        });
    }

    @Override
    protected void onPause() {
        super.onPause();
        //nativeOnPause();
        //gvrApi.pauseTracking();
    }

    @Override
    protected void onStart() {
        super.onStart();
        nativeEnterForeground();
    }

    @Override
    protected void onStop() {
        super.onStop();
        nativeEnterBackground();
    }

    @Override
    protected void onResume() {
        super.onResume();
        //nativeOnResume();
        //gvrApi.resumeTracking();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        nativeOnDestroy();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        // Checks the orientation of the screen
        if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT){
//            Log.d("[VR]", "Portrait, forcing landscape");
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
            if (inVrMode) {
                inVrMode = false;
//                Log.d("[VR]", "Starting VR exit");
                nativeOnExitVr();                
            } else {
                Log.w("[VR]", "Portrait detected but not in VR mode. Should not happen");
            }
        }
    }

    public void openUrlInAndroidWebView(String urlString) {
        Log.d("openUrl", "Received in open " + urlString);
        Intent openUrlIntent = new Intent(this, WebViewActivity.class);
        openUrlIntent.putExtra(WebViewActivity.WEB_VIEW_ACTIVITY_EXTRA_URL, urlString);
        startActivity(openUrlIntent);
    }

    /**
     * Called when view focus is changed
     */
    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            final int uiOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

            getWindow().getDecorView().setSystemUiVisibility(uiOptions);
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (intent.hasExtra(DOMAIN_URL)) {
            nativeGotoUrl(intent.getStringExtra(DOMAIN_URL));
        }
        nativeGoBackFromAndroidActivity();
    }

    public void openAndroidActivity(String activityName) {
        switch (activityName) {
            case "Home": {
                Intent intent = new Intent(this, HomeActivity.class);
                intent.putExtra(HomeActivity.PARAM_NOT_START_INTERFACE_ACTIVITY, true);
                startActivity(intent);
                break;
            }
            default: {
                Log.w(TAG, "Could not open activity by name " + activityName);
                break;
            }
        }
    }

    @Override
    public void onBackPressed() {
        openAndroidActivity("Home");
    }
}