package io.highfidelity.questInterface;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;

import io.highfidelity.oculus.OculusMobileActivity;
import io.highfidelity.utils.HifiUtils;

public class InterfaceActivity extends OculusMobileActivity {
    public static final String DOMAIN_URL = "url";
    private static final String TAG = "Interface";
    public static final String EXTRA_ARGS = "args";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        HifiUtils.upackAssets(getAssets(), getCacheDir().getAbsolutePath());
        super.onCreate(savedInstanceState);
    }
}
