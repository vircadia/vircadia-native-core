package io.highfidelity.hifiinterface.view;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.support.v4.content.ContextCompat;
import android.support.v4.graphics.drawable.RoundedBitmapDrawable;
import android.support.v4.graphics.drawable.RoundedBitmapDrawableFactory;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.squareup.picasso.Callback;
import com.squareup.picasso.Picasso;

import java.util.ArrayList;
import java.util.List;

import io.highfidelity.hifiinterface.R;
import io.highfidelity.hifiinterface.provider.UsersProvider;

/**
 * Created by cduarte on 6/13/18.
 */

public class UserListAdapter extends RecyclerView.Adapter<UserListAdapter.ViewHolder> {

    private UsersProvider mProvider;
    private LayoutInflater mInflater;
    private Context mContext;
    private List<User> mUsers = new ArrayList<>();
    private ItemClickListener mClickListener;
    private AdapterListener mAdapterListener;

    private static List<User> USERS_TMP_CACHE;

    public UserListAdapter(Context c, UsersProvider usersProvider) {
        mContext = c;
        mInflater = LayoutInflater.from(mContext);
        mProvider = usersProvider;
    }

    public void setListener(AdapterListener adapterListener) {
        mAdapterListener = adapterListener;
    }

    public void startLoad() {
        useTmpCachedUsers();
        loadUsers();
    }

    private void useTmpCachedUsers() {
        synchronized (this) {
            if (USERS_TMP_CACHE != null && USERS_TMP_CACHE.size() > 0) {
                mUsers = new ArrayList<>(USERS_TMP_CACHE.size());
                mUsers.addAll(USERS_TMP_CACHE);
                notifyDataSetChanged();
                if (mAdapterListener != null) {
                    if (mUsers.isEmpty()) {
                        mAdapterListener.onEmptyAdapter(false);
                    } else {
                        mAdapterListener.onNonEmptyAdapter(false);
                    }
                }
            }
        }
    }

    public void loadUsers() {
        mProvider.retrieve(new UsersProvider.UsersCallback() {
            @Override
            public void retrieveOk(List<User> users) {
                mUsers = new ArrayList<>(users);
                notifyDataSetChanged();

                synchronized (this) {
                    USERS_TMP_CACHE = new ArrayList<>(mUsers.size());
                    USERS_TMP_CACHE.addAll(mUsers);

                    if (mAdapterListener != null) {
                        if (mUsers.isEmpty()) {
                            mAdapterListener.onEmptyAdapter(true);
                        } else {
                            mAdapterListener.onNonEmptyAdapter(true);
                        }
                    }
                }
            }

            @Override
            public void retrieveError(Exception e, String message) {
                Log.e("[USERS]", message, e);
                if (mAdapterListener != null) {
                    mAdapterListener.onError(e, message);
                }
            }
        });
    }

    @Override
    public UserListAdapter.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = mInflater.inflate(R.layout.user_item, parent, false);
        return new UserListAdapter.ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(UserListAdapter.ViewHolder holder, int position) {
        User aUser = mUsers.get(position);
        holder.mUsername.setText(aUser.name);

        holder.mOnlineInfo.setVisibility(aUser.online? View.VISIBLE : View.GONE);
        holder.mLocation.setText("- " + aUser.locationName); // Bring info from the API and use it here

        holder.mFriendStar.onBindSet(aUser.name, aUser.connection.equals(UsersProvider.CONNECTION_TYPE_FRIEND));
        Uri uri = Uri.parse(aUser.imageUrl);
        Picasso.get().load(uri).into(holder.mImage, new RoundProfilePictureCallback(holder.mImage));
    }

    private class RoundProfilePictureCallback implements Callback {
        private ImageView mProfilePicture;
        public RoundProfilePictureCallback(ImageView imageView) {
            mProfilePicture = imageView;
        }

        @Override
        public void onSuccess() {
            Bitmap imageBitmap = ((BitmapDrawable) mProfilePicture.getDrawable()).getBitmap();
            RoundedBitmapDrawable imageDrawable = RoundedBitmapDrawableFactory.create(mProfilePicture.getContext().getResources(), imageBitmap);
            imageDrawable.setCircular(true);
            imageDrawable.setCornerRadius(Math.max(imageBitmap.getWidth(), imageBitmap.getHeight()) / 2.0f);
            mProfilePicture.setImageDrawable(imageDrawable);
        }

        @Override
        public void onError(Exception e) {
            mProfilePicture.setImageResource(R.drawable.default_profile_avatar);
        }
    }

    @Override
    public int getItemCount() {
        return mUsers.size();
    }

    public class ToggleWrapper {

        private ViewGroup mFrame;
        private ImageView mImage;
        private boolean mChecked = false;
        private String mUsername;
        private boolean waitingChangeConfirm = false;

        public ToggleWrapper(ViewGroup toggleFrame) {
            mFrame = toggleFrame;
            mImage = toggleFrame.findViewById(R.id.userFavImage);
            mFrame.setOnClickListener(view -> toggle());
        }

        private void refreshUI() {
            mImage.setColorFilter(ContextCompat.getColor(mImage.getContext(),
                    mChecked ? R.color.starSelectedTint : R.color.starUnselectedTint));
        }

        class RollbackUICallback implements UsersProvider.UserActionCallback {

            boolean previousStatus;

            RollbackUICallback(boolean previousStatus) {
                this.previousStatus = previousStatus;
            }

            @Override
            public void requestOk() {
                if (!waitingChangeConfirm) {
                    return;
                }
                mFrame.setClickable(true);
                // nothing to do, new status was set
            }

            @Override
            public void requestError(Exception e, String message) {
                if (!waitingChangeConfirm) {
                    return;
                }
                // new status was not set, rolling back
                mChecked = previousStatus;
                mFrame.setClickable(true);
                refreshUI();
            }

        }

        protected void toggle() {
            // TODO API CALL TO CHANGE
            final boolean previousStatus = mChecked;
            mChecked = !mChecked;
            mFrame.setClickable(false);
            refreshUI();
            waitingChangeConfirm = true;
            if (mChecked) {
                mProvider.addFriend(mUsername, new RollbackUICallback(previousStatus));
            } else {
                mProvider.removeFriend(mUsername, new RollbackUICallback(previousStatus));
            }
        }

        protected void onBindSet(String username, boolean checked) {
            mChecked = checked;
            mUsername = username;
            waitingChangeConfirm = false;
            mFrame.setClickable(true);
            refreshUI();
        }
    }

    public class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener {

        TextView mUsername;
        TextView mOnline;
        View mOnlineInfo;
        TextView mLocation;
        ImageView mImage;
        ToggleWrapper mFriendStar;

        public ViewHolder(View itemView) {
            super(itemView);
            mUsername = itemView.findViewById(R.id.userName);
            mOnline = itemView.findViewById(R.id.userOnline);
            mImage = itemView.findViewById(R.id.userImage);
            mOnlineInfo = itemView.findViewById(R.id.userOnlineInfo);
            mLocation = itemView.findViewById(R.id.userLocation);
            mFriendStar = new ToggleWrapper(itemView.findViewById(R.id.userFav));
            itemView.setOnClickListener(this);
        }

        @Override
        public void onClick(View view) {
            int position = getAdapterPosition();
            if (mClickListener != null) {
                mClickListener.onItemClick(view, position, mUsers.get(position));
            }
        }
    }

    // allows clicks events to be caught
    public void setClickListener(ItemClickListener itemClickListener) {
        this.mClickListener = itemClickListener;
    }

    public interface ItemClickListener {
        void onItemClick(View view, int position, User user);
    }

    public static class User {
        public String name;
        public String imageUrl;
        public String connection;
        public boolean online;

        public String locationName;

        public User() {}
    }

    public interface AdapterListener {
        void onEmptyAdapter(boolean shouldStopRefreshing);
        void onNonEmptyAdapter(boolean shouldStopRefreshing);
        void onError(Exception e, String message);
    }

}
