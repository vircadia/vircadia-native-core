package io.highfidelity.hifiinterface.fragment;

import android.app.Fragment;
import android.content.Context;
import android.os.Bundle;
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

import io.highfidelity.hifiinterface.HifiUtils;
import io.highfidelity.hifiinterface.R;

import static org.qtproject.qt5.android.QtActivityDelegate.ApplicationActive;
import static org.qtproject.qt5.android.QtActivityDelegate.ApplicationInactive;

public class SignupFragment extends Fragment
                            implements  OnBackPressedListener {

    private EditText mEmail;
    private EditText mUsername;
    private EditText mPassword;
    private TextView mError;
    private TextView mActivityText;
    private Button mSignupButton;
    private CheckBox mKeepMeLoggedInCheckbox;

    private ViewGroup mSignupForm;
    private ViewGroup mLoggingInFrame;
    private ViewGroup mLoggedInFrame;

    private boolean mLoginInProgress;
    private boolean mSignupInProgress;
    private boolean mSignupSuccess;

    public native void signup(String email, String username, String password); // move to SignupFragment
    public native void cancelSignup();
    public native void login(String username, String password, boolean keepLoggedIn);
    public native void cancelLogin();

    private SignupFragment.OnSignupInteractionListener mListener;

    public SignupFragment() {
        // Required empty public constructor
    }

    public static SignupFragment newInstance() {
        SignupFragment fragment = new SignupFragment();
        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_signup, container, false);

        mEmail = rootView.findViewById(R.id.email);
        mUsername = rootView.findViewById(R.id.username);
        mPassword = rootView.findViewById(R.id.password);
        mError = rootView.findViewById(R.id.error);
        mSignupButton = rootView.findViewById(R.id.signupButton);
        mActivityText = rootView.findViewById(R.id.activityText);
        mKeepMeLoggedInCheckbox = rootView.findViewById(R.id.keepMeLoggedIn);

        mSignupForm = rootView.findViewById(R.id.signupForm);
        mLoggedInFrame = rootView.findViewById(R.id.loggedInFrame);
        mLoggingInFrame = rootView.findViewById(R.id.loggingInFrame);

        rootView.findViewById(R.id.cancel).setOnClickListener(view -> onCancelSignup());

        mSignupButton.setOnClickListener(view -> signup());

        rootView.findViewById(R.id.getStarted).setOnClickListener(view -> onGetStartedClicked());

        mPassword.setOnEditorActionListener((textView, actionId, keyEvent) -> onPasswordEditorAction(textView, actionId, keyEvent));

        mKeepMeLoggedInCheckbox.setChecked(HifiUtils.getInstance().isKeepingLoggedIn());

        return rootView;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof OnSignupInteractionListener) {
            mListener = (OnSignupInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnSignupInteractionListener");
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

    private boolean onPasswordEditorAction(TextView textView, int actionId, KeyEvent keyEvent) {
        if (actionId == EditorInfo.IME_ACTION_DONE) {
            mSignupButton.performClick();
            return true;
        }
        return false;
    }

    private void onCancelSignup() {
        if (mListener != null) {
            mListener.onCancelSignup();
        }
    }

    public void signup() {
        String email = mEmail.getText().toString().trim();
        String username = mUsername.getText().toString().trim();
        String password = mPassword.getText().toString();
        hideKeyboard();
        if (email.isEmpty() || username.isEmpty() || password.isEmpty()) {
            showError(getString(R.string.signup_email_username_or_password_incorrect));
        } else {
            mSignupButton.setEnabled(false);
            hideError();
            mActivityText.setText(R.string.creating_account);
            showActivityIndicator();
            mSignupInProgress = true;
            mSignupSuccess = false;
            signup(email, username, password);
        }
    }

    private void hideKeyboard() {
        View view = getActivity().getCurrentFocus();
        if (view != null) {
            InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        }
    }

    private void showActivityIndicator() {
        mSignupForm.setVisibility(View.GONE);
        mLoggedInFrame.setVisibility(View.GONE);
        mLoggingInFrame.setVisibility(View.VISIBLE);
    }

    private void showLoggedInMessage() {
        mSignupForm.setVisibility(View.GONE);
        mLoggingInFrame.setVisibility(View.GONE);
        mLoggedInFrame.setVisibility(View.VISIBLE);
    }

    private void showSignupForm() {
        mLoggingInFrame.setVisibility(View.GONE);
        mLoggedInFrame.setVisibility(View.GONE);
        mSignupForm.setVisibility(View.VISIBLE);
    }
    private void showError(String error) {
        mError.setText(error);
        mError.setVisibility(View.VISIBLE);
    }

    private void hideError() {
        mError.setText("");
        mError.setVisibility(View.INVISIBLE);
    }

    public interface OnSignupInteractionListener {
        void onSignupCompleted();
        void onCancelSignup();
    }

    private void onGetStartedClicked() {
        if (mListener != null) {
            mListener.onSignupCompleted();
        }
    }


    public void handleSignupCompleted() {
        mSignupInProgress = false;
        String username = mUsername.getText().toString().trim();
        String password = mPassword.getText().toString();
        getActivity().runOnUiThread(() -> {
            mActivityText.setText(R.string.logging_in);
        });
        mLoginInProgress = true;
        boolean keepUserLoggedIn = mKeepMeLoggedInCheckbox.isChecked();
        login(username, password, keepUserLoggedIn);
    }

    public void handleSignupFailed(String error) {
        mSignupInProgress = false;
        getActivity().runOnUiThread(() -> {
            mSignupButton.setEnabled(true);
            showSignupForm();
            mError.setText(error);
            mError.setVisibility(View.VISIBLE);
        });
    }

    public void handleLoginCompleted(boolean success) {
        mLoginInProgress = false;
        getActivity().runOnUiThread(() -> {
            mSignupButton.setEnabled(true);
            if (success) {
                mSignupSuccess = true;
                showLoggedInMessage();
            } else {
                // Registration was successful but login failed.
                // Let the user to login manually
                mListener.onCancelSignup();
                showSignupForm();
            }
        });
    }

    @Override
    public boolean doBack() {
        if (mSignupInProgress) {
           cancelSignup();
        } else if (mLoginInProgress) {
            cancelLogin();
        }

        if (mSignupInProgress || mLoginInProgress) {
            showSignupForm();
            mLoginInProgress = false;
            mSignupInProgress = false;
            mSignupButton.setEnabled(true);
            return true;
        } else if (mSignupSuccess) {
            onGetStartedClicked();
            return true;
        } else {
            return false;
        }
    }
}
