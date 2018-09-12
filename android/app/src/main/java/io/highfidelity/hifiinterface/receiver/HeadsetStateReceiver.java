package io.highfidelity.hifiinterface.receiver;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.util.Log;

public class HeadsetStateReceiver extends BroadcastReceiver {

    private native void notifyHeadsetOn(boolean pluggedIn);

    @Override
    public void onReceive(Context context, Intent intent) {
        AudioManager audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        notifyHeadsetOn(audioManager.isWiredHeadsetOn());
    }
}
