package io.highfidelity.hifiinterface.view;

import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import io.highfidelity.hifiinterface.R;

/**
 * Created by Gabriel Calero & Cristian Duarte on 3/20/18.
 */
public class DomainAdapter extends RecyclerView.Adapter<DomainAdapter.ViewHolder> {

    private Context mContext;
    private LayoutInflater mInflater;
    private ItemClickListener mClickListener;

    public DomainAdapter(Context c) {
        mContext = c;
        this.mInflater = LayoutInflater.from(mContext);
    }

    public class Domain {
        public String name;
        public String url;
        public Integer thumbnail;
        Domain(String name, String url, Integer thumbnail) {
            this.name = name;
            this.thumbnail = thumbnail;
            this.url = url;
        }
    }

    // references to our domains
    private Domain[] mDomains = {
            new Domain("dev-mobile-master", "hifi://dev-mobile-master", 0),
            new Domain("dev-mobile", "hifi://dev-mobile", 0),
            new Domain("dev-welcome", "hifi://dev-welcome", 0),
    };

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = mInflater.inflate(R.layout.domain_view, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {
        // TODO
        //holder.thumbnail.setImageResource(mDomains[position].thumbnail);
        holder.mDomainName.setText(mDomains[position].name);
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
}
