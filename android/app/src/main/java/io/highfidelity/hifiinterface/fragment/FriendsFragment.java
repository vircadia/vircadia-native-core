package io.highfidelity.hifiinterface.fragment;


import android.app.Fragment;
import android.os.Bundle;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import io.highfidelity.hifiinterface.R;
import io.highfidelity.hifiinterface.view.UserListAdapter;

public class FriendsFragment extends Fragment {

    public native boolean nativeIsLoggedIn();

    public native String nativeGetAccessToken();

    private RecyclerView mUsersView;
    private UserListAdapter mUsersAdapter;

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

        Log.d("[USERS]", "token : [" + accessToken + "]");

        mUsersView = rootView.findViewById(R.id.rvUsers);
        int numberOfColumns = 1;
        GridLayoutManager gridLayoutMgr = new GridLayoutManager(getContext(), numberOfColumns);
        mUsersView.setLayoutManager(gridLayoutMgr);
        mUsersAdapter = new UserListAdapter(getContext(), accessToken);
        mUsersView.setAdapter(mUsersAdapter);

        return rootView;
    }

}
