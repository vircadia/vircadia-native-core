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

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

import io.highfidelity.hifiinterface.R;
import io.highfidelity.hifiinterface.provider.DomainProvider;
import io.highfidelity.hifiinterface.provider.UserStoryDomainProvider;

/**
 * Created by Gabriel Calero & Cristian Duarte on 3/20/18.
 */
public class DomainAdapter extends RecyclerView.Adapter<DomainAdapter.ViewHolder> {

    private static final String TAG = "HiFi Interface";
    private Context mContext;
    private LayoutInflater mInflater;
    private ItemClickListener mClickListener;
    private String mProtocol;
    private UserStoryDomainProvider domainProvider;
    private AdapterListener mAdapterListener;

    // references to our domains
    private Domain[] mDomains = {};

    public DomainAdapter(Context c, String protocol) {
        mContext = c;
        this.mInflater = LayoutInflater.from(mContext);
        mProtocol = protocol;
        domainProvider = new UserStoryDomainProvider(mProtocol);
        loadDomains("");
    }

    public void setListener(AdapterListener adapterListener) {
        mAdapterListener = adapterListener;
    }

    public void loadDomains(String filterText) {
        domainProvider.retrieve(filterText, new DomainProvider.DomainCallback() {
            @Override
            public void retrieveOk(List<Domain> domain) {
                mDomains = new Domain[domain.size()];
                mDomains = domain.toArray(mDomains);
                notifyDataSetChanged();
                if (mAdapterListener != null) {
                    if (mDomains.length == 0) {
                        mAdapterListener.onEmptyAdapter();
                    } else {
                        mAdapterListener.onNonEmptyAdapter();
                    }
                }
            }

            @Override
            public void retrieveError(Exception e, String message) {
                Log.e("DOMAINS", message, e);
                if (mAdapterListener != null) mAdapterListener.onError(e, message);
            }
        });
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = mInflater.inflate(R.layout.domain_view, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {
        // TODO
        //holder.thumbnail.setImageResource(mDomains[position].thumbnail);
        Domain domain = mDomains[position];
        holder.mDomainName.setText(domain.name);
        Uri uri = Uri.parse(domain.thumbnail);
        Picasso.get().load(uri).into(holder.mThumbnail);
    }

    @Override
    public int getItemCount() {
        return mDomains.length;
    }

    public class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener {
        TextView mDomainName;
        ImageView mThumbnail;

        ViewHolder(View itemView) {
            super(itemView);
            mThumbnail = (ImageView) itemView.findViewById(R.id.domainThumbnail);
            mDomainName = (TextView) itemView.findViewById(R.id.domainName);
            itemView.setOnClickListener(this);
        }

        @Override
        public void onClick(View view) {
            int position = getAdapterPosition();
            if (mClickListener != null) mClickListener.onItemClick(view, position, mDomains[position]);
        }
    }

    // allows clicks events to be caught
    public void setClickListener(ItemClickListener itemClickListener) {
        this.mClickListener = itemClickListener;
    }
    // parent activity will implement this method to respond to click events
    public interface ItemClickListener {
        void onItemClick(View view, int position, Domain domain);
    }

    public static class Domain {
        public String name;
        public String url;
        public String thumbnail;
        public Domain(String name, String url, String thumbnail) {
            this.name = name;
            this.thumbnail = thumbnail;
            this.url = url;
        }
    }

    public interface AdapterListener {
        void onEmptyAdapter();
        void onNonEmptyAdapter();
        void onError(Exception e, String message);
    }
}
