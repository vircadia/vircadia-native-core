package io.highfidelity.hifiinterface.fragment;


import android.app.Fragment;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.v4.widget.SwipeRefreshLayout;
import android.support.v7.app.AlertDialog;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.sothree.slidinguppanel.SlidingUpPanelLayout;

import io.highfidelity.hifiinterface.R;
import io.highfidelity.hifiinterface.provider.EndpointUsersProvider;
import io.highfidelity.hifiinterface.provider.UsersProvider;
import io.highfidelity.hifiinterface.view.UserListAdapter;

public class FriendsFragment extends Fragment {

    public native String nativeGetAccessToken();

    private RecyclerView mUsersView;
    private View mUserActions;
    private UserListAdapter mUsersAdapter;
    private SlidingUpPanelLayout mSlidingUpPanelLayout;
    private EndpointUsersProvider mUsersProvider;
    private String mSelectedUsername;

    private OnHomeInteractionListener mListener;
    private SwipeRefreshLayout mSwipeRefreshLayout;

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
        mUsersProvider = new EndpointUsersProvider(accessToken);

        Log.d("[USERS]", "token : [" + accessToken + "]");

        mSwipeRefreshLayout = rootView.findViewById(R.id.swipeRefreshLayout);

        mUsersView = rootView.findViewById(R.id.rvUsers);
        int numberOfColumns = 1;
        GridLayoutManager gridLayoutMgr = new GridLayoutManager(getContext(), numberOfColumns);
        mUsersView.setLayoutManager(gridLayoutMgr);

        mUsersAdapter = new UserListAdapter(getContext(), mUsersProvider);
        mSwipeRefreshLayout.setRefreshing(true);

        mUserActions = rootView.findViewById(R.id.userActionsLayout);

        mSlidingUpPanelLayout = rootView.findViewById(R.id.sliding_layout);
        mSlidingUpPanelLayout.setPanelHeight(0);

        rootView.findViewById(R.id.userActionDelete).setOnClickListener(view -> onRemoveConnectionClick());

        rootView.findViewById(R.id.userActionVisit).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mListener != null && mSelectedUsername != null) {
                    mListener.onVisitUserSelected(mSelectedUsername);
                }
            }
        });

        mUsersAdapter.setClickListener(new UserListAdapter.ItemClickListener() {
            @Override
            public void onItemClick(View view, int position, UserListAdapter.User user) {
                // 1. 'select' user
                mSelectedUsername = user.name;
                // ..
                // 2. adapt options
                // ..
                rootView.findViewById(R.id.userActionVisit).setVisibility(user.online ? View.VISIBLE : View.GONE);
                // 3. show
                mSlidingUpPanelLayout.setPanelState(SlidingUpPanelLayout.PanelState.EXPANDED);
            }
        });

        mUsersAdapter.setListener(new UserListAdapter.AdapterListener() {
            @Override
            public void onEmptyAdapter(boolean shouldStopRefreshing) {
                if (shouldStopRefreshing) {
                    mSwipeRefreshLayout.setRefreshing(false);
                }
            }

            @Override
            public void onNonEmptyAdapter(boolean shouldStopRefreshing) {
                if (shouldStopRefreshing) {
                    mSwipeRefreshLayout.setRefreshing(false);
                }
            }

            @Override
            public void onError(Exception e, String message) {
                mSwipeRefreshLayout.setRefreshing(false);
            }
        });

        mUsersView.setAdapter(mUsersAdapter);

        mUsersAdapter.startLoad();

        mSlidingUpPanelLayout.setFadeOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mSlidingUpPanelLayout.setPanelState(SlidingUpPanelLayout.PanelState.COLLAPSED);
                mSelectedUsername = null;
            }
        });

        mSwipeRefreshLayout.setOnRefreshListener(() -> mUsersAdapter.loadUsers());

        return rootView;
    }

    private void onRemoveConnectionClick() {
        if (mSelectedUsername == null) {
            return;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setMessage("Remove '" + mSelectedUsername + "' from People?");
        builder.setPositiveButton("Remove", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {
                mUsersProvider.removeConnection(mSelectedUsername, new UsersProvider.UserActionCallback() {
                    @Override
                    public void requestOk() {
                        mSlidingUpPanelLayout.setPanelState(SlidingUpPanelLayout.PanelState.COLLAPSED);
                        mSelectedUsername = null;
                        mUsersAdapter.loadUsers();
                    }

                    @Override
                    public void requestError(Exception e, String message) {
                        // CLD: Show error message?
                    }
                });
            }
        });
        builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {
                // Cancelled, nothing to do
            }
        });
        builder.show();
    }

    /**
     * Processes the back pressed event and returns true if it was managed by this Fragment
     * @return
     */
    public boolean onBackPressed() {
        if (mSlidingUpPanelLayout.getPanelState().equals(SlidingUpPanelLayout.PanelState.EXPANDED)) {
            mSlidingUpPanelLayout.setPanelState(SlidingUpPanelLayout.PanelState.COLLAPSED);
            mSelectedUsername = null;
            return true;
        } else {
            return false;
        }
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof OnHomeInteractionListener) {
            mListener = (OnHomeInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnHomeInteractionListener");
        }
    }

    public interface OnHomeInteractionListener {
        void onVisitUserSelected(String username);
    }
}
