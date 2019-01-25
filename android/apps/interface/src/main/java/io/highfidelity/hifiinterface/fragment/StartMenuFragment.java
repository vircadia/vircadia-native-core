package io.highfidelity.hifiinterface.fragment;

import android.app.Fragment;
import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import io.highfidelity.hifiinterface.R;

public class StartMenuFragment extends Fragment {

    private String TAG = "HighFidelity";
    private StartMenuInteractionListener mListener;

    public StartMenuFragment() {
        // Required empty public constructor
    }

    public static StartMenuFragment newInstance() {
        StartMenuFragment fragment = new StartMenuFragment();
        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View rootView = inflater.inflate(R.layout.fragment_login_menu, container, false);
        rootView.findViewById(R.id.signupButton).setOnClickListener(view -> {
            if (mListener != null) {
                mListener.onSignupButtonClicked();
            }
        });

        rootView.findViewById(R.id.loginButton).setOnClickListener(view -> {
            if (mListener != null) {
                mListener.onLoginButtonClicked();
            }
        });

        rootView.findViewById(R.id.steamLoginButton).setOnClickListener(view -> {
            if (mListener != null) {
                mListener.onSteamLoginButtonClicked();
            }
        });

        rootView.findViewById(R.id.takeMeInWorld).setOnClickListener(view -> {
            if (mListener != null) {
                mListener.onSkipLoginClicked();
            }
        });



        return rootView;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof StartMenuInteractionListener) {
            mListener = (StartMenuInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement StartMenuInteractionListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

    /**
     * This interface must be implemented by activities that contain this
     * fragment to allow an interaction in this fragment to be communicated
     * to the activity and potentially other fragments contained in that
     * activity.
     * <p>
     * See the Android Training lesson <a href=
     * "http://developer.android.com/training/basics/fragments/communicating.html"
     * >Communicating with Other Fragments</a> for more information.
     */
    public interface StartMenuInteractionListener {
        void onSignupButtonClicked();
        void onLoginButtonClicked();
        void onSkipLoginClicked();
        void onSteamLoginButtonClicked();
    }
}
