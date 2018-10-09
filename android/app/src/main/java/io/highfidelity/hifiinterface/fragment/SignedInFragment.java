package io.highfidelity.hifiinterface.fragment;

import android.app.Fragment;
import android.content.Context;
import android.os.Bundle;
import android.text.Html;
import android.text.Spanned;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;

import io.highfidelity.hifiinterface.R;

public class SignedInFragment extends Fragment {

    private Button mGetStartedButton;
    private OnSignedInInteractionListener mListener;

    public SignedInFragment() {
        // Required empty public constructor
    }

    public static SignedInFragment newInstance() {
        SignedInFragment fragment = new SignedInFragment();
        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_signedin, container, false);
        mGetStartedButton = rootView.findViewById(R.id.getStarted);

        mGetStartedButton.setOnClickListener(view -> {
            getStarted();
        });

        return rootView;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof SignedInFragment.OnSignedInInteractionListener) {
            mListener = (SignedInFragment.OnSignedInInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnSignedInInteractionListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

    public void getStarted() {
        if (mListener != null) {
            mListener.onGettingStarted();
        }
    }

    public interface OnSignedInInteractionListener {
        void onGettingStarted();
    }

}
