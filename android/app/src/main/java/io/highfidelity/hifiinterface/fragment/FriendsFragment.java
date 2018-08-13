package io.highfidelity.hifiinterface.fragment;


import android.app.Fragment;
import android.os.Bundle;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.sothree.slidinguppanel.SlidingUpPanelLayout;

import io.highfidelity.hifiinterface.R;
import io.highfidelity.hifiinterface.view.UserListAdapter;

public class FriendsFragment extends Fragment {

    public native boolean nativeIsLoggedIn();

    public native String nativeGetAccessToken();

    private RecyclerView mUsersView;
    private View mUserActions;
    private UserListAdapter mUsersAdapter;
    private SlidingUpPanelLayout mSlidingUpPanelLayout;

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

        mUserActions = rootView.findViewById(R.id.userActionsLayout);

        mSlidingUpPanelLayout = rootView.findViewById(R.id.sliding_layout);
        mSlidingUpPanelLayout.setPanelHeight(0);
        mUsersAdapter.setClickListener(new UserListAdapter.ItemClickListener() {
            @Override
            public void onItemClick(View view, int position, UserListAdapter.User user) {
                // 1. 'select' user
                // ..
                // 2. adapt options
                // ..
                // 3. show
                mSlidingUpPanelLayout.setPanelState(SlidingUpPanelLayout.PanelState.EXPANDED);
            }
        });
        mUsersView.setAdapter(mUsersAdapter);

        mSlidingUpPanelLayout.setFadeOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mSlidingUpPanelLayout.setPanelState(SlidingUpPanelLayout.PanelState.COLLAPSED);
            }
        });

        return rootView;
    }

    /**
     * Processes the back pressed event and returns true if it was managed by this Fragment
     * @return
     */
    public boolean onBackPressed() {
        if (mSlidingUpPanelLayout.getPanelState().equals(SlidingUpPanelLayout.PanelState.EXPANDED)) {
            mSlidingUpPanelLayout.setPanelState(SlidingUpPanelLayout.PanelState.COLLAPSED);
            return true;
        } else {
            return false;
        }
    }
}
