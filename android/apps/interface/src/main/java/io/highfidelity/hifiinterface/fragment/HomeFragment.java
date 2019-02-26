package io.highfidelity.hifiinterface.fragment;

import android.app.Fragment;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.widget.SwipeRefreshLayout;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import io.highfidelity.hifiinterface.HifiUtils;
import io.highfidelity.hifiinterface.R;
import io.highfidelity.hifiinterface.view.DomainAdapter;

public class HomeFragment extends Fragment {

    private DomainAdapter mDomainAdapter;
    private RecyclerView mDomainsView;
    private TextView searchNoResultsView;
    private ImageView mSearchIconView;
    private ImageView mClearSearch;
    private EditText mSearchView;


    private OnHomeInteractionListener mListener;
    private SwipeRefreshLayout mSwipeRefreshLayout;

    public native String nativeGetLastLocation();

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

        searchNoResultsView = rootView.findViewById(R.id.searchNoResultsView);
        mSwipeRefreshLayout = rootView.findViewById(R.id.swipeRefreshLayout);

        mDomainsView = rootView.findViewById(R.id.rvDomains);
        int numberOfColumns = 1;
        GridLayoutManager gridLayoutMgr = new GridLayoutManager(getContext(), numberOfColumns);
        mDomainsView.setLayoutManager(gridLayoutMgr);
        mDomainAdapter = new DomainAdapter(getContext(), HifiUtils.getInstance().protocolVersionSignature(), nativeGetLastLocation());
        mSwipeRefreshLayout.setRefreshing(true);
        mDomainAdapter.setClickListener((view, position, domain) -> {
            new Handler(getActivity().getMainLooper()).postDelayed(() -> {
                if (mListener != null) {
                    mListener.onSelectedDomain(domain.url);
                }
            }, 400); // a delay so the ripple effect can be seen
        });
        mDomainAdapter.setListener(new DomainAdapter.AdapterListener() {
            @Override
            public void onEmptyAdapter(boolean shouldStopRefreshing) {
                searchNoResultsView.setText(R.string.search_no_results);
                searchNoResultsView.setVisibility(View.VISIBLE);
                mDomainsView.setVisibility(View.GONE);
                if (shouldStopRefreshing) {
                    mSwipeRefreshLayout.setRefreshing(false);
                }
            }

            @Override
            public void onNonEmptyAdapter(boolean shouldStopRefreshing) {
                searchNoResultsView.setVisibility(View.GONE);
                mDomainsView.setVisibility(View.VISIBLE);
                if (shouldStopRefreshing) {
                    mSwipeRefreshLayout.setRefreshing(false);
                }
            }

            @Override
            public void onError(Exception e, String message) {

            }
        });
        mDomainsView.setAdapter(mDomainAdapter);
        mDomainAdapter.startLoad();

        mSearchView = rootView.findViewById(R.id.searchView);
        mSearchIconView = rootView.findViewById(R.id.search_mag_icon);
        mClearSearch = rootView.findViewById(R.id.search_clear);

        getActivity().getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);

        return rootView;
    }

    @Override
    public void onStart() {
        super.onStart();
        mSearchView.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {}

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {}

            @Override
            public void afterTextChanged(Editable editable) {
                mDomainAdapter.loadDomains(editable.toString(), false);
                if (editable.length() > 0) {
                    mSearchIconView.setVisibility(View.GONE);
                    mClearSearch.setVisibility(View.VISIBLE);
                } else {
                    mSearchIconView.setVisibility(View.VISIBLE);
                    mClearSearch.setVisibility(View.GONE);
                }
            }
        });
        mSearchView.setOnKeyListener((view, i, keyEvent) -> {
            if (i == KeyEvent.KEYCODE_ENTER) {
                String urlString = mSearchView.getText().toString();
                if (!urlString.trim().isEmpty()) {
                    urlString = HifiUtils.getInstance().sanitizeHifiUrl(urlString);
                }
                if (mListener != null) {
                    mListener.onSelectedDomain(urlString);
                }
                return true;
            }
            return false;
        });

        mClearSearch.setOnClickListener(view -> onSearchClear(view));

        mSwipeRefreshLayout.setOnRefreshListener(new SwipeRefreshLayout.OnRefreshListener() {
            @Override
            public void onRefresh() {
                mDomainAdapter.loadDomains(mSearchView.getText().toString(), true);
            }
        });
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

    public void onSearchClear(View view) {
        mSearchView.setText("");
    }


}
