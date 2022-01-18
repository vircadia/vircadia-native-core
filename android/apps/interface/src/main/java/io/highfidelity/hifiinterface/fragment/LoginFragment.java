package io.highfidelity.hifiinterface.fragment;

import android.app.Activity;
import android.app.Fragment;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;

import org.qtproject.qt5.android.QtNative;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.Random;

import io.highfidelity.hifiinterface.BuildConfig;
import io.highfidelity.hifiinterface.HifiUtils;
import io.highfidelity.hifiinterface.R;
import io.highfidelity.hifiinterface.WebViewActivity;

import static org.qtproject.qt5.android.QtActivityDelegate.ApplicationActive;
import static org.qtproject.qt5.android.QtActivityDelegate.ApplicationInactive;

public class LoginFragment extends Fragment
                            implements  OnBackPressedListener {

    private static final String ARG_USE_OAUTH = "use_oauth";
    private static final String TAG = "Interface";

    private final String OAUTH_CLIENT_ID = BuildConfig.OAUTH_CLIENT_ID;
    private final String OAUTH_REDIRECT_URI = BuildConfig.OAUTH_REDIRECT_URI;
    private final String OAUTH_AUTHORIZE_BASE_URL = "https://metaverse.vircadia.com/live/oauth/authorize";
    private static final int OAUTH_AUTHORIZE_REQUEST = 1;

    private EditText mUsername;
    private EditText mPassword;
    private TextView mError;
    private Button mLoginButton;
    private CheckBox mKeepMeLoggedInCheckbox;
    private ViewGroup mLoginForm;
    private ViewGroup mLoggingInFrame;
    private ViewGroup mLoggedInFrame;
    private boolean mLoginInProgress;
    private boolean mLoginSuccess;
    private boolean mUseOauth;
    private String mOauthState;

    public native void login(String username, String password, boolean keepLoggedIn);
    private native void retrieveAccessToken(String authCode, String clientId, String clientSecret, String redirectUri);

    public native void cancelLogin();

    private LoginFragment.OnLoginInteractionListener mListener;

    public LoginFragment() {
        // Required empty public constructor
    }

    public static LoginFragment newInstance(boolean useOauth) {
        LoginFragment fragment = new LoginFragment();
        Bundle args = new Bundle();
        args.putBoolean(ARG_USE_OAUTH, useOauth);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (getArguments() != null) {
            mUseOauth = getArguments().getBoolean(ARG_USE_OAUTH, false);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_login, container, false);

        mUsername = rootView.findViewById(R.id.username);
        mPassword = rootView.findViewById(R.id.password);
        mError = rootView.findViewById(R.id.error);
        mLoginButton = rootView.findViewById(R.id.loginButton);
        mLoginForm = rootView.findViewById(R.id.loginForm);
        mLoggingInFrame = rootView.findViewById(R.id.loggingInFrame);
        mLoggedInFrame = rootView.findViewById(R.id.loggedInFrame);
        mKeepMeLoggedInCheckbox = rootView.findViewById(R.id.keepMeLoggedIn);

        rootView.findViewById(R.id.forgotPassword).setOnClickListener(view -> onForgotPasswordClicked());

        rootView.findViewById(R.id.cancel).setOnClickListener(view -> onCancelLogin());

        rootView.findViewById(R.id.getStarted).setOnClickListener(view -> onGetStartedClicked());

        mLoginButton.setOnClickListener(view -> onLoginButtonClicked());

        rootView.findViewById(R.id.takeMeInWorld).setOnClickListener(view -> skipLogin());
        mPassword.setOnEditorActionListener((textView, actionId, keyEvent) -> onPasswordEditorAction(textView, actionId, keyEvent));

        mKeepMeLoggedInCheckbox.setChecked(HifiUtils.getInstance().isKeepingLoggedIn());

        if (mUseOauth) {
            openWebForAuthorization();
        } else {
            showLoginForm();
        }
        return rootView;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof OnLoginInteractionListener) {
            mListener = (OnLoginInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnLoginInteractionListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

    @Override
    public void onResume() {
        super.onResume();
        // This hack intends to keep Qt threads running even after the app comes from background
        QtNative.setApplicationState(ApplicationActive);
    }

    @Override
    public void onStop() {
        super.onStop();
        // Leave the Qt app paused
        QtNative.setApplicationState(ApplicationInactive);
        hideKeyboard();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == OAUTH_AUTHORIZE_REQUEST) {
            if (resultCode == Activity.RESULT_OK) {
                String authCode = data.getStringExtra(WebViewActivity.RESULT_OAUTH_CODE);
                String state = data.getStringExtra(WebViewActivity.RESULT_OAUTH_STATE);
                if (state != null && state.equals(mOauthState) && mListener != null) {
                    mOauthState = null;
                    showActivityIndicator();
                    mLoginInProgress = true;
                    retrieveAccessToken(authCode, BuildConfig.OAUTH_CLIENT_ID, BuildConfig.OAUTH_CLIENT_SECRET, BuildConfig.OAUTH_REDIRECT_URI);
                }
            } else {
                onCancelLogin();
            }
        }

    }

    private void onCancelLogin() {
        if (mListener != null) {
            mListener.onCancelLogin();
        }
    }

    private boolean onPasswordEditorAction(TextView textView, int actionId, KeyEvent keyEvent) {
        if (actionId == EditorInfo.IME_ACTION_DONE) {
            mLoginButton.performClick();
            return true;
        }
        return false;
    }

    private void skipLogin() {
        if (mListener != null) {
            mListener.onSkipLoginClicked();
        }
    }

    private void onGetStartedClicked() {
        if (mListener != null) {
            mListener.onLoginCompleted();
        }
    }

    public void onLoginButtonClicked() {
        String username = mUsername.getText().toString().trim();
        String password = mPassword.getText().toString();
        hideKeyboard();
        if (username.isEmpty() || password.isEmpty()) {
            showError(getString(R.string.login_username_or_password_incorrect));
        } else {
            mLoginButton.setEnabled(false);
            hideError();
            showActivityIndicator();
            mLoginInProgress = true;
            mLoginSuccess = false;
            boolean keepUserLoggedIn = mKeepMeLoggedInCheckbox.isChecked();
            login(username, password, keepUserLoggedIn);
        }
    }

    private void hideKeyboard() {
        View view = getActivity().getCurrentFocus();
        if (view != null) {
            InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        }
    }

    private void onForgotPasswordClicked() {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://metaverse.vircadia.com/live/users/password/new"));
        startActivity(intent);
    }

    private void showActivityIndicator() {
        mLoginForm.setVisibility(View.GONE);
        mLoggedInFrame.setVisibility(View.GONE);
        mLoggingInFrame.setVisibility(View.VISIBLE);
        mLoggingInFrame.bringToFront();
    }

    private void showLoginForm() {
        mLoggingInFrame.setVisibility(View.GONE);
        mLoggedInFrame.setVisibility(View.GONE);
        mLoginForm.setVisibility(View.VISIBLE);
        mLoginForm.bringToFront();
    }

    private void showLoggedInMessage() {
        mLoginForm.setVisibility(View.GONE);
        mLoggingInFrame.setVisibility(View.GONE);
        mLoggedInFrame.setVisibility(View.VISIBLE);
        mLoggedInFrame.bringToFront();
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
        mLoginInProgress = false;
        getActivity().runOnUiThread(() -> {
            mLoginButton.setEnabled(true);
            if (success) {
                mLoginSuccess = true;
                showLoggedInMessage();
            } else {
                if (!mUseOauth) {
                    showLoginForm();
                    showError(getString(R.string.login_username_or_password_incorrect));
                } else {
                    openWebForAuthorization();
                }
            }
        });
    }

    @Override
    public boolean doBack() {
        if (mLoginInProgress) {
            cancelLogin();
            showLoginForm();
            mLoginInProgress = false;
            mLoginButton.setEnabled(true);
            return true;
        } else if (mLoginSuccess) {
            onGetStartedClicked();
            return true;
        } else {
            return false;
        }
    }

    private void updateOauthState() {
        // as we only use oauth for steam that's ok for now
        mOauthState = "steam-" + Long.toString(new Random().nextLong());
    }

    private String buildAuthorizeUrl() {
        StringBuilder sb = new StringBuilder(OAUTH_AUTHORIZE_BASE_URL);
        sb.append("?client_id=").append(OAUTH_CLIENT_ID);
        try {
            String redirectUri = URLEncoder.encode(OAUTH_REDIRECT_URI, "utf-8");
            sb.append("&redirect_uri=").append(redirectUri);
        } catch (UnsupportedEncodingException e) {
            Log.e(TAG, "Cannot build oauth autorization url", e);
        }
        sb.append("&response_type=code&scope=owner");
        sb.append("&state=").append(mOauthState);
        return sb.toString();
    }

    private void openWebForAuthorization() {
        Intent openUrlIntent = new Intent(getActivity(), WebViewActivity.class);
        updateOauthState();
        openUrlIntent.putExtra(WebViewActivity.WEB_VIEW_ACTIVITY_EXTRA_URL, buildAuthorizeUrl());
        openUrlIntent.putExtra(WebViewActivity.WEB_VIEW_ACTIVITY_EXTRA_CLEAR_COOKIES, true);
        startActivityForResult(openUrlIntent, OAUTH_AUTHORIZE_REQUEST);
    }


    public interface OnLoginInteractionListener {
        void onLoginCompleted();
        void onCancelLogin();
        void onSkipLoginClicked();
    }

}
