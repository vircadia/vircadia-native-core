package io.highfidelity.hifiinterface.view;

import android.content.Context;
import android.net.Uri;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

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
        holder.mOnline.setText(aUser.online?"ONLINE":"OFFLINE");
        Uri uri = Uri.parse(aUser.imageUrl);
        Picasso.get().load(uri).into(holder.mImage);
    }

    @Override
    public int getItemCount() {
        return mUsers.size();
    }

    public class ViewHolder extends RecyclerView.ViewHolder {

        TextView mUsername;
        TextView mOnline;
        ImageView mImage;

        public ViewHolder(View itemView) {
            super(itemView);
            mUsername = itemView.findViewById(R.id.userName);
            mOnline = itemView.findViewById(R.id.userOnline);
            mImage = itemView.findViewById(R.id.userImage);
        }
    }

    public static class User {
        public String name;
        public String imageUrl;
        public String connection;
        public boolean online;

        public User() {}
    }
}
