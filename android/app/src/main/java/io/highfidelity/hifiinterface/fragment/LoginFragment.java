package io.highfidelity.hifiinterface.fragment;

import android.app.Activity;
import android.app.Fragment;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import io.highfidelity.hifiinterface.R;

public class LoginFragment extends Fragment {

    private EditText mUsername;
    private EditText mPassword;
    private TextView mError;
    private TextView mForgotPassword;
    private Button mLoginButton;

    private ProgressDialog mDialog;

    public native void nativeLogin(String username, String password, Activity usernameChangedListener);

    private LoginFragment.OnLoginInteractionListener mListener;

    public LoginFragment() {
        // Required empty public constructor
    }

    public static LoginFragment newInstance() {
        LoginFragment fragment = new LoginFragment();
        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_login, container, false);

        mUsername = rootView.findViewById(R.id.username);
        mPassword = rootView.findViewById(R.id.password);
        mError = rootView.findViewById(R.id.error);
        mLoginButton = rootView.findViewById(R.id.loginButton);
        mForgotPassword = rootView.findViewById(R.id.forgotPassword);

        mUsername.addTextChangedListener(new TextWatcher() {
            boolean ignoreNextChange = false;
            boolean hadBlankSpace = false;
            @Override
            public void beforeTextChanged(CharSequence charSequence, int start, int count, int after) {
                hadBlankSpace = charSequence.length() > 0 && charSequence.charAt(charSequence.length()-1) == ' ';
            }

            @Override
            public void onTextChanged(CharSequence charSequence, int start, int count, int after) {

            }

            @Override
            public void afterTextChanged(Editable editable) {
                if (!ignoreNextChange) {
                    ignoreNextChange = true;
                    boolean spaceFound = false;
                    for (int i = 0; i < editable.length(); i++) {
                        if (editable.charAt(i) == ' ') {
                            spaceFound=true;
                            editable.delete(i, i + 1);
                            i--;
                        }
                    }

                    if (hadBlankSpace && !spaceFound && editable.length() > 0) {
                        editable.delete(editable.length()-1, editable.length());
                    }

                    editable.append(' ');
                    ignoreNextChange = false;
                }

            }
        });


        mLoginButton.setOnClickListener(view -> login());

        mForgotPassword.setOnClickListener(view -> forgotPassword());

        mPassword.setOnEditorActionListener(
                (textView, actionId, keyEvent) -> {
                    if (actionId == EditorInfo.IME_ACTION_DONE) {
                        mLoginButton.performClick();
                        return true;
                    }
                    return false;
                });
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
    public void onStop() {
        super.onStop();
        cancelActivityIndicator();
        hideKeyboard();
    }

    public void login() {
        String username = mUsername.getText().toString().trim();
        String password = mPassword.getText().toString();
        hideKeyboard();
        if (username.isEmpty() || password.isEmpty()) {
            showError(getString(R.string.login_username_or_password_incorrect));
        } else {
            mLoginButton.setEnabled(false);
            hideError();
            showActivityIndicator();
            nativeLogin(username, password, getActivity());
        }
    }

    private void hideKeyboard() {
        View view = getActivity().getCurrentFocus();
        if (view != null) {
            InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        }
    }

    private void forgotPassword() {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://highfidelity.com/users/password/new"));
        startActivity(intent);
    }

    private void showActivityIndicator() {
        if (mDialog == null) {
            mDialog = new ProgressDialog(getContext());
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
        Log.d("[LOGIN]", "handleLoginCompleted " + success);
        getActivity().runOnUiThread(() -> {
            mLoginButton.setEnabled(true);
            cancelActivityIndicator();
            if (success) {
                if (mListener != null) {
                    mListener.onLoginCompleted();
                }
            } else {
                showError(getString(R.string.login_username_or_password_incorrect));
            }
        });
    }

    public interface OnLoginInteractionListener {
        void onLoginCompleted();
    }

}
