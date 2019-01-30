package io.highfidelity.frameplayer;

import android.content.Intent;
import android.os.Bundle;

import io.highfidelity.oculus.OculusMobileActivity;

public class QuestRenderActivity extends OculusMobileActivity {
    @Override
    public void onCreate(Bundle savedState) {
        super.onCreate(savedState);
        startActivity(new Intent(this, QuestQtActivity.class));
    }
}
