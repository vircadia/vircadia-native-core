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

import java.util.Arrays;
import java.util.List;

import io.highfidelity.hifiinterface.R;
import io.highfidelity.hifiinterface.provider.DomainProvider;
import io.highfidelity.hifiinterface.provider.UserStoryDomainProvider;

/**
 * Created by Gabriel Calero & Cristian Duarte on 3/20/18.
 */
public class DomainAdapter extends RecyclerView.Adapter<DomainAdapter.ViewHolder> {

    private static final String TAG = "HiFi Interface";
    private static final String DEFAULT_THUMBNAIL_PLACE = "android.resource://io.highfidelity.hifiinterface/" + R.drawable.domain_placeholder;
    private Context mContext;
    private LayoutInflater mInflater;
    private ItemClickListener mClickListener;
    private String mProtocol;
    private String mLastLocation;
    private UserStoryDomainProvider domainProvider;
    private AdapterListener mAdapterListener;

    // references to our domains
    private Domain[] mDomains = {};

    private static Domain[] DOMAINS_TMP_CACHE = {};

    public DomainAdapter(Context c, String protocol, String lastLocation) {
        mContext = c;
        this.mInflater = LayoutInflater.from(mContext);
        mProtocol = protocol;
        mLastLocation = lastLocation;
        domainProvider = new UserStoryDomainProvider(mProtocol);
    }

    public void setListener(AdapterListener adapterListener) {
        mAdapterListener = adapterListener;
    }

    public void startLoad() {
        useTmpCachedDomains();
        loadDomains("", true);
    }

    private void useTmpCachedDomains() {
        synchronized (this) {
            if (DOMAINS_TMP_CACHE != null && DOMAINS_TMP_CACHE.length > 0) {
                mDomains = Arrays.copyOf(DOMAINS_TMP_CACHE, DOMAINS_TMP_CACHE.length);
                notifyDataSetChanged();
                if (mAdapterListener != null) {
                    if (mDomains.length == 0) {
                        mAdapterListener.onEmptyAdapter(false);
                    } else {
                        mAdapterListener.onNonEmptyAdapter(false);
                    }
                }
            }
        }
    }

    public void loadDomains(String filterText, boolean forceRefresh) {
        domainProvider.retrieve(filterText, new DomainProvider.DomainCallback() {
            @Override
            public void retrieveOk(List<Domain> domain) {
                if (filterText.length() == 0) {
                    addLastLocation(domain);
                }

                overrideDefaultThumbnails(domain);

                mDomains = new Domain[domain.size()];
                synchronized (this) {
                    domain.toArray(mDomains);
                    if (filterText.isEmpty()) {
                        DOMAINS_TMP_CACHE = Arrays.copyOf(mDomains, mDomains.length);
                    }
                    notifyDataSetChanged();
                    if (mAdapterListener != null) {
                        if (mDomains.length == 0) {
                            mAdapterListener.onEmptyAdapter(true);
                        } else {
                            mAdapterListener.onNonEmptyAdapter(true);
                        }
                    }
                }
            }

            @Override
            public void retrieveError(Exception e, String message) {
                Log.e("DOMAINS", message, e);
                if (mAdapterListener != null) mAdapterListener.onError(e, message);
            }
        }, forceRefresh);
    }

    private void overrideDefaultThumbnails(List<Domain> domain) {
        for (Domain d : domain) {
            // we override the default picture added in the server by an android specific version
            if (d.thumbnail != null &&
                    d.thumbnail.endsWith("assets/places/thumbnail-default-place-e5a3f33e773ab699495774990a562f9f7693dc48ef90d8be6985c645a0280f75.png")) {
                d.thumbnail = DEFAULT_THUMBNAIL_PLACE;
            }
        }
    }

    private void addLastLocation(List<Domain> domain) {
        Domain lastVisitedDomain = new Domain(mContext.getString(R.string.your_last_location), mLastLocation, DEFAULT_THUMBNAIL_PLACE);
        if (!mLastLocation.isEmpty() && mLastLocation.contains("://")) {
            int startIndex = mLastLocation.indexOf("://");
            int endIndex = mLastLocation.indexOf("/", startIndex + 3);
            String toSearch = mLastLocation.substring(0, endIndex + 1).toLowerCase();
            for (Domain d : domain) {
                if (d.url.toLowerCase().startsWith(toSearch)) {
                    lastVisitedDomain.thumbnail = d.thumbnail;
                }
            }
        }
        domain.add(0, lastVisitedDomain);
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = mInflater.inflate(R.layout.domain_view, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {
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
        void onEmptyAdapter(boolean shouldStopRefreshing);
        void onNonEmptyAdapter(boolean shouldStopRefreshing);
        void onError(Exception e, String message);
    }
}
