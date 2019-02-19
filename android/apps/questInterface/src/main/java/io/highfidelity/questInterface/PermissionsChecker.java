package io.highfidelity.questInterface;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;

import io.highfidelity.oculus.OculusMobileActivity;
import io.highfidelity.utils.HifiUtils;

public class PermissionsChecker extends Activity {
    private static final int REQUEST_PERMISSIONS = 20;
    private static final String TAG = PermissionsChecker.class.getName();
    private static final String[] REQUIRED_PERMISSIONS = new String[]{
        Manifest.permission.READ_EXTERNAL_STORAGE,
        Manifest.permission.WRITE_EXTERNAL_STORAGE,
        Manifest.permission.RECORD_AUDIO,
        Manifest.permission.CAMERA
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestAppPermissions(REQUIRED_PERMISSIONS,REQUEST_PERMISSIONS);
    }

    public void requestAppPermissions(final String[] requestedPermissions,
                                      final int requestCode) {
        int permissionCheck = PackageManager.PERMISSION_GRANTED;
        boolean shouldShowRequestPermissionRationale = false;
        for (String permission : requestedPermissions) {
            permissionCheck = permissionCheck + checkSelfPermission(permission);
            shouldShowRequestPermissionRationale = shouldShowRequestPermissionRationale || shouldShowRequestPermissionRationale(permission);
        }
        if (permissionCheck != PackageManager.PERMISSION_GRANTED) {
            System.out.println("Permission was not granted. Ask for permissions");
            if (shouldShowRequestPermissionRationale) {
                requestPermissions(requestedPermissions, requestCode);
            } else {
                requestPermissions(requestedPermissions, requestCode);
            }
        } else {
            System.out.println("Launching the other activity..");
            launchActivityWithPermissions();
        }
    }

    private void launchActivityWithPermissions() {
        startActivity(new Intent(this, InterfaceActivity.class));
        finish();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        int permissionCheck = PackageManager.PERMISSION_GRANTED;
        for (int permission : grantResults) {
            permissionCheck = permissionCheck + permission;
        }
        if ((grantResults.length > 0) && permissionCheck == PackageManager.PERMISSION_GRANTED) {
            launchActivityWithPermissions();
        } else if (grantResults.length > 0) {
            System.out.println("User has deliberately denied Permissions. Launching anyways");
            launchActivityWithPermissions();
        }
    }
}
