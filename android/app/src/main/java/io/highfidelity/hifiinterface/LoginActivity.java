package io.highfidelity.hifiinterface;

import android.app.ProgressDialog;
import android.support.annotation.MainThread;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

public class LoginActivity extends AppCompatActivity {

    public native void nativeLogin(String username, String password);

    private EditText mUsername;
    private EditText mPassword;
    private TextView mError;
    private Button mLoginButton;

    private ProgressDialog mDialog;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);

        mUsername = findViewById(R.id.username);
        mPassword = findViewById(R.id.password);
        mError = findViewById(R.id.error);
        mLoginButton = findViewById(R.id.loginButton);

        mPassword.setOnEditorActionListener(
            new TextView.OnEditorActionListener() {
                @Override
                public boolean onEditorAction(TextView textView, int actionId, KeyEvent keyEvent) {
                    if (actionId == EditorInfo.IME_ACTION_DONE) {
                        mLoginButton.performClick();
                        return true;
                    }
                    return false;
                }
            });

    }

    @Override
    protected void onStop() {
        super.onStop();
        cancelActivityIndicator();
    }

    public void login(View view) {
        String username = mUsername.getText().toString();
        String password = mPassword.getText().toString();
        if (username.isEmpty() || password.isEmpty()) {
            showError(getString(R.string.login_username_or_password_incorrect));
        } else {
            mLoginButton.setEnabled(false);
            hideError();
            showActivityIndicator();
            nativeLogin(username, password);
        }
    }

    private void showActivityIndicator() {
        if (mDialog == null) {
            mDialog = new ProgressDialog(this);
        }
        mDialog.setMessage(getString(R.string.logging_in));
        mDialog.setCancelable(false);
        mDialog.show();
    }

    private void cancelActivityIndicator() {
        if (mDialog != null) {
            mDialog.cancel();
        }
    }
    private void showError(String error) {
        mError.setText(error);
        mError.setVisibility(View.VISIBLE);
    }

    private void hideError() {
        mError.setText("");
        mError.setVisibility(View.INVISIBLE);
    }

    public void handleLoginCompleted(boolean success) {
        runOnUiThread(() -> {
            mLoginButton.setEnabled(true);
            cancelActivityIndicator();
            if (success) {
                finish();
            } else {
                showError(getString(R.string.login_username_or_password_incorrect));
            }
        });
    }

}
