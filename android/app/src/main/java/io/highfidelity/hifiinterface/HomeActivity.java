package io.highfidelity.hifiinterface;

import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.design.widget.NavigationView;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.Toolbar;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

import io.highfidelity.hifiinterface.view.DomainAdapter;

public class HomeActivity extends AppCompatActivity implements NavigationView.OnNavigationItemSelectedListener {

    public native boolean nativeIsLoggedIn();
    public native void nativeLogout();
    public native String nativeGetDisplayName();

    /**
     * Set this intent extra param to NOT start a new InterfaceActivity after a domain is selected"
     */
    //public static final String PARAM_NOT_START_INTERFACE_ACTIVITY = "not_start_interface_activity";

    public static final int ENTER_DOMAIN_URL = 1;

    private DomainAdapter mDomainAdapter;
    private DrawerLayout mDrawerLayout;
    private NavigationView mNavigationView;
    private RecyclerView mDomainsView;
    private TextView searchNoResultsView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_home);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        toolbar.setTitleTextAppearance(this, R.style.HomeActionBarTitleStyle);
        setSupportActionBar(toolbar);

        ActionBar actionbar = getSupportActionBar();
        actionbar.setDisplayHomeAsUpEnabled(true);
        actionbar.setHomeAsUpIndicator(R.drawable.ic_menu);

        mDrawerLayout = findViewById(R.id.drawer_layout);

        mNavigationView = (NavigationView) findViewById(R.id.nav_view);
        mNavigationView.setNavigationItemSelectedListener(this);

        searchNoResultsView = findViewById(R.id.searchNoResultsView);

        mDomainsView = findViewById(R.id.rvDomains);
        int numberOfColumns = 1;
        GridLayoutManager gridLayoutMgr = new GridLayoutManager(this, numberOfColumns);
        mDomainsView.setLayoutManager(gridLayoutMgr);
        mDomainAdapter = new DomainAdapter(this, HifiUtils.getInstance().protocolVersionSignature());
        mDomainAdapter.setClickListener(new DomainAdapter.ItemClickListener() {

            @Override
            public void onItemClick(View view, int position, DomainAdapter.Domain domain) {
                gotoDomain(domain.url);
            }
        });
        mDomainAdapter.setListener(new DomainAdapter.AdapterListener() {
            @Override
            public void onEmptyAdapter() {
                searchNoResultsView.setText(R.string.search_no_results);
                searchNoResultsView.setVisibility(View.VISIBLE);
                mDomainsView.setVisibility(View.GONE);
            }

            @Override
            public void onNonEmptyAdapter() {
                searchNoResultsView.setVisibility(View.GONE);
                mDomainsView.setVisibility(View.VISIBLE);
            }

            @Override
            public void onError(Exception e, String message) {

            }
        });
        mDomainsView.setAdapter(mDomainAdapter);

        EditText searchView = findViewById(R.id.searchView);
        int searchPlateId = searchView.getContext().getResources().getIdentifier("android:id/search_plate", null, null);
        View searchPlate = searchView.findViewById(searchPlateId);
        if (searchPlate != null) {
            searchPlate.setBackgroundColor (Color.TRANSPARENT);
            int searchTextId = searchPlate.getContext ().getResources ().getIdentifier ("android:id/search_src_text", null, null);
            TextView searchTextView = searchView.findViewById(searchTextId);
            searchTextView.setTextAppearance(R.style.SearchText);
        }
        searchView.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {}

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {}

            @Override
            public void afterTextChanged(Editable editable) {
                mDomainAdapter.loadDomains(editable.toString());
            }
        });
        searchView.setOnKeyListener((view, i, keyEvent) -> {
            if (i == KeyEvent.KEYCODE_ENTER) {
                String urlString = searchView.getText().toString();
                if (!urlString.trim().isEmpty()) {
                    urlString = HifiUtils.getInstance().sanitizeHifiUrl(urlString);
                }
                gotoDomain(urlString);
                return true;
            }
            return false;
        });
        updateLoginMenu();

    }

    private void updateLoginMenu() {
        TextView loginOption = findViewById(R.id.login);
        TextView logoutOption = findViewById(R.id.logout);
        if (nativeIsLoggedIn()) {
            loginOption.setVisibility(View.GONE);
            logoutOption.setVisibility(View.VISIBLE);
        } else {
            loginOption.setVisibility(View.VISIBLE);
            logoutOption.setVisibility(View.GONE);
        }
    }

    private void gotoDomain(String domainUrl) {
        Intent intent = new Intent(HomeActivity.this, InterfaceActivity.class);
        intent.putExtra(InterfaceActivity.DOMAIN_URL, domainUrl);
        HomeActivity.this.finish();
        intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        startActivity(intent);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        //getMenuInflater().inflate(R.menu.menu_home, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        switch (id) {
            case android.R.id.home:
                mDrawerLayout.openDrawer(GravityCompat.START);
                return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    public boolean onNavigationItemSelected(@NonNull MenuItem item) {
        switch(item.getItemId()) {
            case R.id.action_goto:
                Intent i = new Intent(this, GotoActivity.class);
                startActivityForResult(i, ENTER_DOMAIN_URL);
                return true;
        }
        return false;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == ENTER_DOMAIN_URL && resultCode == RESULT_OK) {
            gotoDomain(data.getStringExtra(GotoActivity.PARAM_DOMAIN_URL));
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
        updateLoginMenu();
    }

    public void onLoginClicked(View view) {
        Intent intent = new Intent(this, LoginActivity.class);
        startActivity(intent);
    }

    public void onLogoutClicked(View view) {
        nativeLogout();
        updateLoginMenu();
    }
}
