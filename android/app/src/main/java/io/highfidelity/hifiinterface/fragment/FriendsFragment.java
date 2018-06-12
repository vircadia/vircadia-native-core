package io.highfidelity.hifiinterface.fragment;


import android.app.Fragment;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import io.highfidelity.hifiinterface.R;

public class FriendsFragment extends Fragment {

    public native boolean nativeIsLoggedIn();

    public native String nativeGetAccessToken();

    public FriendsFragment() {
        // Required empty public constructor
    }

    public static FriendsFragment newInstance() {
        FriendsFragment fragment = new FriendsFragment();
        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_friends, container, false);

        String accessToken = nativeGetAccessToken();

        Log.d("[TOKENX]", "token : [" + accessToken + "]");

        return rootView;
    }

}
