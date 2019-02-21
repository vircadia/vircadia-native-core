package io.highfidelity.questInterface;

import android.os.Bundle;
import io.highfidelity.oculus.OculusMobileActivity;
import io.highfidelity.utils.HifiUtils;

public class InterfaceActivity extends OculusMobileActivity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        HifiUtils.upackAssets(getAssets(), getCacheDir().getAbsolutePath());
        super.onCreate(savedInstanceState);
    }
}
