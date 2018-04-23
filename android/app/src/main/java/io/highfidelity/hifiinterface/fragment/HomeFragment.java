package io.highfidelity.hifiinterface.fragment;

import android.app.Fragment;
import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.SearchView;
import android.widget.TabHost;
import android.widget.TabWidget;
import android.widget.TextView;

import io.highfidelity.hifiinterface.R;
import io.highfidelity.hifiinterface.view.DomainAdapter;

public class HomeFragment extends Fragment {

    public static final int ENTER_DOMAIN_URL = 1;

    private DomainAdapter domainAdapter;

    private OnHomeInteractionListener mListener;

    public HomeFragment() {
        // Required empty public constructor
    }

    public static HomeFragment newInstance() {
        HomeFragment fragment = new HomeFragment();
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (getArguments() != null) {
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_home, container, false);

        TabHost tabs = rootView.findViewById(R.id.tabhost);
        tabs.setup();

        TabHost.TabSpec spec=tabs.newTabSpec("featured");
        spec.setContent(R.id.featured);
        spec.setIndicator(getString(R.string.featured));
        tabs.addTab(spec);

        spec = tabs.newTabSpec("popular");
        spec.setContent(R.id.popular);
        spec.setIndicator(getString(R.string.popular));
        tabs.addTab(spec);

        spec = tabs.newTabSpec("bookmarks");
        spec.setContent(R.id.bookmarks);
        spec.setIndicator(getString(R.string.bookmarks));
        tabs.addTab(spec);

        tabs.setCurrentTab(0);

        TabWidget tabwidget=tabs.getTabWidget();
        for(int i=0; i<tabwidget.getChildCount(); i++){
            TextView tv= tabwidget.getChildAt(i).findViewById(android.R.id.title);
            tv.setTextAppearance(R.style.TabText);
        }


        RecyclerView domainsView = rootView.findViewById(R.id.rvDomains);
        int numberOfColumns = 1;
        GridLayoutManager gridLayoutMgr = new GridLayoutManager(getContext(), numberOfColumns);
        domainsView.setLayoutManager(gridLayoutMgr);
        domainAdapter = new DomainAdapter(getContext());
        domainAdapter.setClickListener((view, position, domain) -> mListener.onSelectedDomain(domain.url));
        domainsView.setAdapter(domainAdapter);

        SearchView searchView = rootView.findViewById(R.id.searchView);
        int searchPlateId = searchView.getContext().getResources().getIdentifier("android:id/search_plate", null, null);
        View searchPlate = searchView.findViewById(searchPlateId);
        if (searchPlate != null) {
            searchPlate.setBackgroundColor (Color.TRANSPARENT);
            int searchTextId = searchPlate.getContext ().getResources ().getIdentifier ("android:id/search_src_text", null, null);
            TextView searchTextView = searchView.findViewById(searchTextId);
            searchTextView.setTextAppearance(R.style.SearchText);
        }
        return rootView;
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

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

    public interface OnHomeInteractionListener {
        void onSelectedDomain(String domainUrl);
    }

}
