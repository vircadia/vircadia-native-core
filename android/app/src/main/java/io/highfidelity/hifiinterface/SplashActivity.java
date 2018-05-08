package io.highfidelity.hifiinterface;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;

public class SplashActivity extends Activity {

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
    protected void onStop() {
        super.onStop();
    }

    public void onAppLoadedComplete() {
        // Give interface more time so textures don't fail(got deleted) on Adreno (joystick)
        new Handler(getMainLooper()).postDelayed(() -> {
            startActivity(new Intent(this, HomeActivity.class));
            new Handler(getMainLooper()).postDelayed(() -> SplashActivity.this.finish(), 1000);
        }, 500);
    }
}
