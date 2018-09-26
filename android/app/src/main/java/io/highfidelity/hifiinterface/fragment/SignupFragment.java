package io.highfidelity.hifiinterface.fragment;

import android.app.Activity;
import android.app.Fragment;
import android.app.ProgressDialog;
import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import org.qtproject.qt5.android.QtNative;

import io.highfidelity.hifiinterface.R;

import static org.qtproject.qt5.android.QtActivityDelegate.ApplicationActive;
import static org.qtproject.qt5.android.QtActivityDelegate.ApplicationInactive;

public class SignupFragment extends Fragment {

    private EditText mEmail;
    private EditText mUsername;
    private EditText mPassword;
    private TextView mError;
    private TextView mCancelButton;

    private Button mSignupButton;

    private ProgressDialog mDialog;

    public native void nativeSignup(String email, String username, String password); // move to SignupFragment
    public native void nativeCancelSignup();
    public native void nativeLogin(String username, String password, Activity usernameChangedListener);
    public native void nativeCancelLogin();

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
        mCancelButton = rootView.findViewById(R.id.cancelButton);

        mSignupButton.setOnClickListener(view -> signup());
        mCancelButton.setOnClickListener(view -> login());
        mPassword.setOnEditorActionListener(
                (textView, actionId, keyEvent) -> {
                    if (actionId == EditorInfo.IME_ACTION_DONE) {
                        mSignupButton.performClick();
                        return true;
                    }
                    return false;
                });
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
        cancelActivityIndicator();
        // Leave the Qt app paused
        QtNative.setApplicationState(ApplicationInactive);
        hideKeyboard();
    }

    private void login() {
        if (mListener != null) {
            mListener.onLoginRequested();
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
            showActivityIndicator();
            nativeSignup(email, username, password);
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
        if (mDialog == null) {
            mDialog = new ProgressDialog(getContext());
        }
        mDialog.setMessage(getString(R.string.creating_account));
        mDialog.setCancelable(true);
        mDialog.setOnCancelListener(dialogInterface -> {
            nativeCancelSignup();
            cancelActivityIndicator();
            mSignupButton.setEnabled(true);
        });
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

    public interface OnSignupInteractionListener {
        void onSignupCompleted();
        void onLoginRequested();
    }

    public void handleSignupCompleted() {
        String username = mUsername.getText().toString().trim();
        String password = mPassword.getText().toString();
        mDialog.setMessage(getString(R.string.logging_in));
        mDialog.setCancelable(true);
        mDialog.setOnCancelListener(dialogInterface -> {
            nativeCancelLogin();
            cancelActivityIndicator();
            if (mListener != null) {
                mListener.onLoginRequested();
            }
        });
        mDialog.show();
        nativeLogin(username, password, getActivity());
    }

    public void handleSignupFailed(String error) {
        getActivity().runOnUiThread(() -> {
            mSignupButton.setEnabled(true);
            cancelActivityIndicator();
            mError.setText(error);
            mError.setVisibility(View.VISIBLE);
        });
    }

    public void handleLoginCompleted(boolean success) {
        getActivity().runOnUiThread(() -> {
            mSignupButton.setEnabled(true);
            cancelActivityIndicator();

            if (success) {
                if (mListener != null) {
                    mListener.onSignupCompleted();
                }
            } else {
                // Registration was successful but login failed.
                // Let the user to login manually
                mListener.onLoginRequested();
            }
        });
    }



}
