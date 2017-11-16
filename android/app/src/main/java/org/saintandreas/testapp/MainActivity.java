package org.saintandreas.testapp;

import android.app.Activity;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.View;

import com.google.vr.ndk.base.AndroidCompat;
import com.google.vr.ndk.base.GvrLayout;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends Activity {
    private final static int IMMERSIVE_STICKY_VIEW_FLAGS = View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
        View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
        View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
        View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
        View.SYSTEM_UI_FLAG_FULLSCREEN |
        View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

    static {
        System.loadLibrary("gvr");
        System.loadLibrary("native-lib");
    }

    private long nativeRenderer;
    //private GvrLayout gvrLayout;
    private GLSurfaceView surfaceView;

    private native long nativeCreateRenderer(ClassLoader appClassLoader, Context context, long nativeGvrContext);
    private native void nativeDestroyRenderer(long renderer);
    private native void nativeInitializeGl(long renderer);
    private native void nativeDrawFrame(long renderer);
    private native void nativeOnTriggerEvent(long renderer);
    private native void nativeOnPause(long renderer);
    private native void nativeOnResume(long renderer);

    class NativeRenderer implements GLSurfaceView.Renderer {
        @Override public void onSurfaceCreated(GL10 gl, EGLConfig config) { nativeInitializeGl(nativeRenderer); }
        @Override public void onSurfaceChanged(GL10 gl, int width, int height) { }
        @Override public void onDrawFrame(GL10 gl) {
            nativeDrawFrame(nativeRenderer);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setImmersiveSticky();
        getWindow()
            .getDecorView()
            .setOnSystemUiVisibilityChangeListener((int visibility)->{
                if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) { setImmersiveSticky(); }
            });

        // gvrLayout = new GvrLayout(this);
        nativeRenderer = nativeCreateRenderer(
            getClass().getClassLoader(),
            getApplicationContext(),
            0);
            //gvrLayout.getGvrApi().getNativeGvrContext());

        surfaceView = new GLSurfaceView(this);
        surfaceView.setEGLContextClientVersion(3);
        surfaceView.setEGLConfigChooser(8, 8, 8, 0, 0, 0);
        surfaceView.setPreserveEGLContextOnPause(true);
        surfaceView.setRenderer(new NativeRenderer());

//        gvrLayout.setPresentationView(surfaceView);
//        setContentView(gvrLayout);
//        if (gvrLayout.setAsyncReprojectionEnabled(true)) {
//            AndroidCompat.setSustainedPerformanceMode(this, true);
//        }
//        AndroidCompat.setVrModeEnabled(this, true);

        setContentView(surfaceView);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        //gvrLayout.shutdown();
        nativeDestroyRenderer(nativeRenderer);
        nativeRenderer = 0;
    }

    @Override
    protected void onPause() {
        surfaceView.queueEvent(()->nativeOnPause(nativeRenderer));
        surfaceView.onPause();
//        gvrLayout.onPause();
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
//        gvrLayout.onResume();
        surfaceView.onResume();
        surfaceView.queueEvent(()->nativeOnResume(nativeRenderer));
    }

    private void setImmersiveSticky() {
        getWindow().getDecorView().setSystemUiVisibility(IMMERSIVE_STICKY_VIEW_FLAGS);
    }
}
