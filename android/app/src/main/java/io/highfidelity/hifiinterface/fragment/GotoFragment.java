package io.highfidelity.hifiinterface.fragment;

import android.app.Fragment;
import android.content.Context;
import android.os.Bundle;
import android.support.v7.widget.AppCompatButton;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

import java.net.URI;
import java.net.URISyntaxException;

import io.highfidelity.hifiinterface.HifiUtils;
import io.highfidelity.hifiinterface.R;

public class GotoFragment extends Fragment {

    private EditText mUrlEditText;
    private AppCompatButton mGoBtn;

    private OnGotoInteractionListener mListener;

    public GotoFragment() {
        // Required empty public constructor
    }

    public static GotoFragment newInstance() {
        GotoFragment fragment = new GotoFragment();
        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_goto, container, false);

        mUrlEditText = rootView.findViewById(R.id.url_text);
        mUrlEditText.setOnKeyListener((view, i, keyEvent) -> {
            if (i == KeyEvent.KEYCODE_ENTER) {
                actionGo();
                return true;
            }
            return false;
        });

        mUrlEditText.setText(HifiUtils.getInstance().getCurrentAddress());

        mGoBtn = rootView.findViewById(R.id.go_btn);

        mGoBtn.setOnClickListener(view -> actionGo());

        return rootView;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof OnGotoInteractionListener) {
            mListener = (OnGotoInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnGotoInteractionListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

    private void actionGo() {
        String urlString = mUrlEditText.getText().toString();
        if (!urlString.trim().isEmpty()) {
            URI uri;
            try {
                uri = new URI(urlString);
            } catch (URISyntaxException e) {
                return;
            }
            if (uri.getScheme() == null || uri.getScheme().isEmpty()) {
                urlString = "hifi://" + urlString;
            }

            mListener.onEnteredDomain(urlString);
        }
    }

    public interface OnGotoInteractionListener {
        void onEnteredDomain(String domainUrl);
    }

}
