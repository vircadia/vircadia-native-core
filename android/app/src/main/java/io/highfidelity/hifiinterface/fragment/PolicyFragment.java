package io.highfidelity.hifiinterface.fragment;

import android.app.Fragment;
import android.os.Bundle;
import android.text.Html;
import android.text.Spanned;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;

import io.highfidelity.hifiinterface.R;

public class PolicyFragment extends Fragment {

    private static final String POLICY_FILE = "privacy_policy.html";

    public PolicyFragment() {
        // Required empty public constructor
    }

    public static PolicyFragment newInstance() {
        PolicyFragment fragment = new PolicyFragment();
        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_policy, container, false);

        TextView policy = rootView.findViewById(R.id.policyText);


        Spanned myStringSpanned = null;
        try {
            myStringSpanned = Html.fromHtml(loadHTMLFromAsset(), null, null);
            policy.setText(myStringSpanned, TextView.BufferType.SPANNABLE);
        } catch (IOException e) {
            policy.setText("N/A");
        }

        return rootView;
    }
    
    public String loadHTMLFromAsset() throws IOException {
        String html = null;
        InputStream is = getContext().getAssets().open(POLICY_FILE);
        int size = is.available();
        byte[] buffer = new byte[size];
        is.read(buffer);
        is.close();
        html = new String(buffer, "UTF-8");
        return html;
    }

}
