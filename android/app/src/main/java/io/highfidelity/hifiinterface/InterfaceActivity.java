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

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.graphics.Point;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Vibrator;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.SlidingDrawer;

import org.qtproject.qt5.android.QtLayout;
import org.qtproject.qt5.android.QtSurface;
import org.qtproject.qt5.android.bindings.QtActivity;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import io.highfidelity.hifiinterface.fragment.WebViewFragment;
import io.highfidelity.hifiinterface.receiver.HeadsetStateReceiver;

/*import com.google.vr.cardboard.DisplaySynchronizer;
import com.google.vr.cardboard.DisplayUtils;
import com.google.vr.ndk.base.GvrApi;*/

public class InterfaceActivity extends QtActivity implements WebViewFragment.OnWebViewInteractionListener {

    public static final String DOMAIN_URL = "url";
    public static final String EXTRA_GOTO_USERNAME = "gotousername";
    private static final String TAG = "Interface";
    private static final int WEB_DRAWER_RIGHT_MARGIN = 262;
    private static final int WEB_DRAWER_BOTTOM_MARGIN = 150;
    private static final int NORMAL_DPI = 160;

    private Vibrator mVibrator;
    private HeadsetStateReceiver headsetStateReceiver;

    //public static native void handleHifiURL(String hifiURLString);
    private native long nativeOnCreate(InterfaceActivity instance, AssetManager assetManager);
    private native void nativeOnDestroy();
    private native void nativeGotoUrl(String url);
    private native void nativeGoToUser(String username);
    private native void nativeBeforeEnterBackground();
    private native void nativeEnterBackground();
    private native void nativeEnterForeground();
    private native long nativeOnExitVr();
    private native void nativeInitAfterAppLoaded();

    private AssetManager assetManager;

    private static boolean inVrMode;

    private boolean nativeEnterBackgroundCallEnqueued = false;
    private SlidingDrawer webSlidingDrawer;
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
        super.isLoading = true;
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

        try {
            PackageInfo pInfo = this.getPackageManager().getPackageInfo(getPackageName(), 0);
            String version = pInfo.versionName;
//            setAppVersion(version);
        } catch (PackageManager.NameNotFoundException e) {
            Log.e("GVR", "Error getting application version", e);
        }

        final View rootView = getWindow().getDecorView().findViewById(android.R.id.content);

        // This is a workaround to hide the menu bar when the virtual keyboard is shown from Qt
        rootView.getViewTreeObserver().addOnGlobalLayoutListener(() -> {
            if (getActionBar() != null && getActionBar().isShowing()) {
                getActionBar().hide();
            }
        });
        startActivity(new Intent(this, SplashActivity.class));
        mVibrator = (Vibrator) this.getSystemService(VIBRATOR_SERVICE);

        FrameLayout mainLayout = findViewById(android.R.id.content);
        LayoutInflater inflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        webSlidingDrawer = (SlidingDrawer) inflater.inflate(R.layout.web_drawer, mainLayout, false);
        QtLayout qtLayout = (QtLayout) mainLayout.getChildAt(0);
        QtLayout.LayoutParams layoutParams = new QtLayout.LayoutParams(webSlidingDrawer.getLayoutParams());
        webSlidingDrawer.setOnDrawerCloseListener(() -> {
            WebViewFragment webViewFragment = (WebViewFragment) getFragmentManager().findFragmentByTag("webViewFragment");
            webViewFragment.close();
        });
        int widthPx = Math.max(size.x, size.y);
        int heightPx = Math.min(size.x, size.y);

        layoutParams.x = (int) (widthPx - WEB_DRAWER_RIGHT_MARGIN * getResources().getDisplayMetrics().xdpi / NORMAL_DPI);
        layoutParams.y = (int) (heightPx - WEB_DRAWER_BOTTOM_MARGIN * getResources().getDisplayMetrics().ydpi / NORMAL_DPI);

        layoutParams.resolveLayoutDirection(View.LAYOUT_DIRECTION_RTL);
        qtLayout.addView(webSlidingDrawer, layoutParams);
        webSlidingDrawer.setVisibility(View.GONE);

        headsetStateReceiver = new HeadsetStateReceiver();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (super.isLoading) {
            nativeEnterBackgroundCallEnqueued = true;
        } else {
            nativeEnterBackground();
        }
        unregisterReceiver(headsetStateReceiver);
        //gvrApi.pauseTracking();
    }

    @Override
    protected void onStart() {
        super.onStart();
        nativeEnterBackgroundCallEnqueued = false;
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    }

    @Override
    protected void onStop() {
        super.onStop();

    }

    @Override
    protected void onResume() {
        super.onResume();
        nativeEnterForeground();
        surfacesWorkaround();
        keepInterfaceRunning = false;
        registerReceiver(headsetStateReceiver, new IntentFilter(Intent.ACTION_HEADSET_PLUG));
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
        surfacesWorkaround();
    }

    private void surfacesWorkaround() {
        FrameLayout fl = findViewById(android.R.id.content);
        if (fl.getChildCount() > 0) {
            QtLayout qtLayout = (QtLayout) fl.getChildAt(0);
            List<QtSurface> surfaces = new ArrayList<>();
            for (int i = 0; i < qtLayout.getChildCount(); i++) {
                Object ch = qtLayout.getChildAt(i);
                if (ch instanceof QtSurface) {
                    surfaces.add((QtSurface) ch);
                }
            }
            if (surfaces.size() > 1) {
                QtSurface s1 = surfaces.get(0);
                QtSurface s2 = surfaces.get(1);
                Integer subLayer1 = 0;
                Integer subLayer2 = 0;
                try {
                    String field;
                    if (android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                        field = "mSubLayer";
                    } else {
                        field = "mWindowType";
                    }
                    Field f = s1.getClass().getSuperclass().getDeclaredField(field);
                    f.setAccessible(true);
                    subLayer1 = (Integer) f.get(s1);
                    subLayer2 = (Integer) f.get(s2);
                    if (subLayer1 < subLayer2) {
                        s1.setVisibility(View.VISIBLE);
                        s2.setVisibility(View.INVISIBLE);
                    } else {
                        s1.setVisibility(View.INVISIBLE);
                        s2.setVisibility(View.VISIBLE);
                    }
                } catch (ReflectiveOperationException e) {
                    Log.e(TAG, "Workaround failed");
                }
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
            webSlidingDrawer.setVisibility(View.GONE);
            nativeGotoUrl(intent.getStringExtra(DOMAIN_URL));
        } else if (intent.hasExtra(EXTRA_GOTO_USERNAME)) {
            webSlidingDrawer.setVisibility(View.GONE);
            nativeGoToUser(intent.getStringExtra(EXTRA_GOTO_USERNAME));
        }
    }

    public void openAndroidActivity(String activityName, boolean backToScene) {
        openAndroidActivity(activityName, backToScene, null);
    }

    public void openAndroidActivity(String activityName, boolean backToScene, HashMap args) {
        switch (activityName) {
            case "Home":
            case "Privacy Policy":
                nativeBeforeEnterBackground();
                Intent intent = new Intent(this, MainActivity.class);
                intent.putExtra(MainActivity.EXTRA_FRAGMENT, activityName);
                intent.putExtra(MainActivity.EXTRA_BACK_TO_SCENE, backToScene);
                startActivity(intent);
                break;
            case "Login":
                nativeBeforeEnterBackground();
                Intent loginIntent = new Intent(this, MainActivity.class);
                loginIntent.putExtra(MainActivity.EXTRA_FRAGMENT, activityName);
                loginIntent.putExtra(MainActivity.EXTRA_BACK_TO_SCENE, backToScene);
                if (args != null && args.containsKey(DOMAIN_URL)) {
                    loginIntent.putExtra(DOMAIN_URL, (String) args.get(DOMAIN_URL));
                }
                startActivity(loginIntent);
                break;
            case "WebView":
                runOnUiThread(() -> {
                    webSlidingDrawer.setVisibility(View.VISIBLE);
                    if (!webSlidingDrawer.isOpened()) {
                        webSlidingDrawer.animateOpen();
                    }
                    if (args != null && args.containsKey(WebViewActivity.WEB_VIEW_ACTIVITY_EXTRA_URL)) {
                        WebViewFragment webViewFragment = (WebViewFragment) getFragmentManager().findFragmentByTag("webViewFragment");
                        webViewFragment.loadUrl((String) args.get(WebViewActivity.WEB_VIEW_ACTIVITY_EXTRA_URL), true);
                        webViewFragment.setToolbarVisible(true);
                        webViewFragment.setCloseAction(() -> {
                            if (webSlidingDrawer.isOpened()) {
                                webSlidingDrawer.animateClose();
                            }
                            webSlidingDrawer.setVisibility(View.GONE);
                        });
                    }
                });
                break;
            default: {
                Log.w(TAG, "Could not open activity by name " + activityName);
                break;
            }
        }
    }

    public void onAppLoadedComplete() {
        super.isLoading = false;
        if (nativeEnterBackgroundCallEnqueued) {
            nativeEnterBackground();
        }
        runOnUiThread(() -> {
            nativeInitAfterAppLoaded();
        });
    }

    public void performHapticFeedback(int duration) {
        if (duration > 0) {
            mVibrator.vibrate(duration);
        }
    }

    @Override
    public void onBackPressed() {
        openAndroidActivity("Home", false);
    }

    @Override
    public void processURL(String url) { }

    @Override
    public void onWebLoaded(String url, WebViewFragment.SafenessLevel safenessLevel) { }

    @Override
    public void onTitleReceived(String title) { }

    @Override
    public void onExpand() {
        keepInterfaceRunning = true;
    }
}
