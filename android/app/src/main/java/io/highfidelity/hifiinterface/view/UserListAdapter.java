package io.highfidelity.hifiinterface.view;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
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
import io.highfidelity.hifiinterface.provider.EndpointUsersProvider;
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

    public UserListAdapter(Context c, String accessToken) {
        mContext = c;
        mInflater = LayoutInflater.from(mContext);
        mProvider = new EndpointUsersProvider(accessToken);
        loadUsers();
    }

    private void loadUsers() {
        mProvider.retrieve(new UsersProvider.UsersCallback() {
            @Override
            public void retrieveOk(List<User> users) {
                mUsers = new ArrayList<>(users);
                notifyDataSetChanged();
            }

            @Override
            public void retrieveError(Exception e, String message) {
                Log.e("[USERS]", message, e);
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
        Uri uri = Uri.parse(aUser.imageUrl);
        holder.mLocation.setText(" - unknown"); // Bring info from the API and use it here
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

    public class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener {

        TextView mUsername;
        TextView mOnline;
        View mOnlineInfo;
        TextView mLocation;
        ImageView mImage;

        public ViewHolder(View itemView) {
            super(itemView);
            mUsername = itemView.findViewById(R.id.userName);
            mOnline = itemView.findViewById(R.id.userOnline);
            mImage = itemView.findViewById(R.id.userImage);
            mOnlineInfo = itemView.findViewById(R.id.userOnlineInfo);
            mLocation = itemView.findViewById(R.id.userLocation);
            itemView.setOnClickListener(this);
        }

        @Override
        public void onClick(View view) {
            int position = getAdapterPosition();
            if (mClickListener != null) mClickListener.onItemClick(view, position, mUsers.get(position));
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

        public User() {}
    }
}
