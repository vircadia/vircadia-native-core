package io.highfidelity.questInterface;

import io.highfidelity.oculus.OculusMobileActivity;

public class InterfaceActivity extends OculusMobileActivity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        HifiUtils.upackAssets(getAssets(), getCacheDir().getAbsolutePath());
        super.onCreate(savedInstanceState);
    }
}
