package io.highfidelity.hifiinterface;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;

public class SplashActivity extends Activity {

    public static final String EXTRA_START_IN_DOMAIN = "start-in-domain";
    private boolean mStartInDomain;

    private native void registerLoadCompleteListener();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_splash);
        registerLoadCompleteListener();
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onResume() {
        super.onResume();
        View decorView = getWindow().getDecorView();
        // Hide the status bar.
        int uiOptions = View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN;
        decorView.setSystemUiVisibility(uiOptions);
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    public void onAppLoadedComplete() {
        if (!mStartInDomain) {
            if (HifiUtils.getInstance().isUserLoggedIn()) {
                startActivity(new Intent(this, MainActivity.class));
            } else {
                Intent menuIntent = new Intent(this, LoginMenuActivity.class);
                menuIntent.putExtra(LoginMenuActivity.EXTRA_FINISH_ON_BACK, true);
                startActivity(menuIntent);
            }
        }
        SplashActivity.this.finish();
    }
   
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean(EXTRA_START_IN_DOMAIN, mStartInDomain);
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        mStartInDomain = savedInstanceState.getBoolean(EXTRA_START_IN_DOMAIN, false);
    }
}
