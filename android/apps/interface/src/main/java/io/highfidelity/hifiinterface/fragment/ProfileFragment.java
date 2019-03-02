package io.highfidelity.hifiinterface.fragment;


import android.app.Fragment;
import android.content.Context;
import android.os.Bundle;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.TextView;

import io.highfidelity.hifiinterface.R;
import io.highfidelity.hifiinterface.provider.AvatarProvider;
import io.highfidelity.hifiinterface.view.AvatarAdapter;

public class ProfileFragment extends Fragment {

    private TextView mDisplayName;

    private Button mOkButton;
    private OnProfileInteractionListener mListener;
    private AvatarProvider mAvatarsProvider;

    private native String getDisplayName();
    private native void setDisplayName(String name);
    private native void setAvatarUrl(String url);

    public ProfileFragment() {
        // Required empty public constructor
    }

    public static ProfileFragment newInstance() {
        ProfileFragment fragment = new ProfileFragment();
        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_profile, container, false);

        mDisplayName = rootView.findViewById(R.id.displayName);
        mDisplayName.setText(getDisplayName());
        mDisplayName.setOnEditorActionListener((textView, actionId, keyEvent) -> onDisplayNameEditorAction(textView, actionId, keyEvent));

        mOkButton = rootView.findViewById(R.id.okButton);
        mOkButton.setOnClickListener(view -> onOkButtonClicked());

        rootView.findViewById(R.id.cancel).setOnClickListener(view -> onCancelProfileEdit());

        RecyclerView avatarsView = rootView.findViewById(R.id.gridview);
        int numberOfColumns = 1;
        mAvatarsProvider = new AvatarProvider(getContext());
        GridLayoutManager gridLayoutMgr = new GridLayoutManager(getContext(), numberOfColumns);
        avatarsView.setLayoutManager(gridLayoutMgr);
        AvatarAdapter avatarAdapter = new AvatarAdapter(getContext(), mAvatarsProvider);
        avatarsView.setAdapter(avatarAdapter);
        avatarAdapter.loadAvatars();

        avatarAdapter.setClickListener((view, position, avatar) -> {
            setAvatarUrl(avatar.avatarUrl);
            if (mListener != null) {
                mListener.onAvatarChosen();
            }
        });
        return rootView;
    }

    private void onOkButtonClicked() {
        setDisplayName(mDisplayName.getText().toString());
        View view = getActivity().getCurrentFocus();
        InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        if (mListener != null) {
            mListener.onCompleteProfileEdit();
        }
    }

    private boolean onDisplayNameEditorAction(TextView textView, int actionId, KeyEvent keyEvent) {
        if (actionId == EditorInfo.IME_ACTION_DONE) {
            mOkButton.performClick();
            return true;
        }
        return false;
    }

    private void onCancelProfileEdit() {
        View view = getActivity().getCurrentFocus();
        InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        if (mListener != null) {
            mListener.onCancelProfileEdit();
        }
    }

    /**
     * Processes the back pressed event and returns true if it was managed by this Fragment
     * @return
     */
    public boolean onBackPressed() {
        return false;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof OnProfileInteractionListener) {
            mListener = (OnProfileInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnProfileInteractionListener");
        }
    }

    public interface OnProfileInteractionListener {
        void onCancelProfileEdit();
        void onCompleteProfileEdit();
        void onAvatarChosen();
    }
}
