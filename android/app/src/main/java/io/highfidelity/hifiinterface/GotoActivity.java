package io.highfidelity.hifiinterface;

import android.content.Intent;
import android.os.Bundle;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.TabHost;
import android.widget.TabWidget;
import android.widget.TextView;
import android.widget.Toast;

import io.highfidelity.hifiinterface.view.DomainAdapter;

public class GotoActivity extends AppCompatActivity {

    private DomainAdapter domainAdapter;
    private DrawerLayout mDrawerLayout;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_goto);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        ActionBar actionbar = getSupportActionBar();
        actionbar.setDisplayHomeAsUpEnabled(true);
        actionbar.setHomeAsUpIndicator(R.drawable.ic_menu);

        mDrawerLayout = findViewById(R.id.drawer_layout);

        TabHost tabs=(TabHost)findViewById(R.id.tabhost);
        tabs.setup();

        TabHost.TabSpec spec=tabs.newTabSpec("featured");
        spec.setContent(R.id.featured);
        spec.setIndicator(getString(R.string.featured));
        tabs.addTab(spec);

        spec=tabs.newTabSpec("popular");
        spec.setContent(R.id.popular);
        spec.setIndicator(getString(R.string.popular));
        tabs.addTab(spec);

        spec=tabs.newTabSpec("bookmarks");
        spec.setContent(R.id.bookmarks);
        spec.setIndicator(getString(R.string.bookmarks));
        tabs.addTab(spec);

        tabs.setCurrentTab(0);

        TabWidget tabwidget=tabs.getTabWidget();
        for(int i=0;i<tabwidget.getChildCount();i++){
            TextView tv=(TextView) tabwidget.getChildAt(i).findViewById(android.R.id.title);
            tv.setTextColor(getColor(R.color.tabs));
        }


        RecyclerView domainsView = findViewById(R.id.rvDomains);
        int numberOfColumns = 1;
        GridLayoutManager gridLayoutMgr = new GridLayoutManager(this, numberOfColumns);
        domainsView.setLayoutManager(gridLayoutMgr);
        domainAdapter = new DomainAdapter(this);
        domainAdapter.setClickListener(new DomainAdapter.ItemClickListener() {

            @Override
            public void onItemClick(View view, int position, DomainAdapter.Domain domain) {
                Toast.makeText(GotoActivity.this, "" + domain.url,
                        Toast.LENGTH_SHORT).show();
                Intent intent = new Intent(GotoActivity.this, InterfaceActivity.class);
                intent.putExtra(InterfaceActivity.DOMAIN_URL, domain.url);
                GotoActivity.this.finish();
                startActivity(intent);
            }
        });
        domainsView.setAdapter(domainAdapter);

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        //getMenuInflater().inflate(R.menu.menu_goto, menu);
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
            case R.id.action_settings:
                return true;
        }

        return super.onOptionsItemSelected(item);
    }
}
