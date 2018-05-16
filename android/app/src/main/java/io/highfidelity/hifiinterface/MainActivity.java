package io.highfidelity.hifiinterface;

import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.design.widget.NavigationView;
import android.support.v4.content.ContextCompat;
import android.support.v4.graphics.drawable.RoundedBitmapDrawable;
import android.support.v4.graphics.drawable.RoundedBitmapDrawableFactory;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.TextView;

import com.squareup.picasso.Callback;
import com.squareup.picasso.Picasso;

import io.highfidelity.hifiinterface.fragment.HomeFragment;
import io.highfidelity.hifiinterface.fragment.LoginFragment;
import io.highfidelity.hifiinterface.fragment.PolicyFragment;
import io.highfidelity.hifiinterface.task.DownloadProfileImageTask;

public class MainActivity extends AppCompatActivity implements NavigationView.OnNavigationItemSelectedListener,
                                                                LoginFragment.OnLoginInteractionListener,
                                                                HomeFragment.OnHomeInteractionListener {

    private static final int PROFILE_PICTURE_PLACEHOLDER = R.drawable.default_profile_avatar;
    public static final String DEFAULT_FRAGMENT = "Home";
    public static final String EXTRA_FRAGMENT = "fragment";
    public static final String EXTRA_BACK_TO_SCENE = "backToScene";

    private String TAG = "HighFidelity";

    public native boolean nativeIsLoggedIn();
    public native void nativeLogout();
    public native String nativeGetDisplayName();

    private DrawerLayout mDrawerLayout;
    private NavigationView mNavigationView;
    private ImageView mProfilePicture;
    private TextView mDisplayName;
    private View mLoginPanel;
    private View mProfilePanel;
    private TextView mLogoutOption;

    private boolean backToScene;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mNavigationView = findViewById(R.id.nav_view);
        mNavigationView.setNavigationItemSelectedListener(this);

        mLoginPanel = mNavigationView.getHeaderView(0).findViewById(R.id.loginPanel);
        mProfilePanel = mNavigationView.getHeaderView(0).findViewById(R.id.profilePanel);

        mLogoutOption = mNavigationView.findViewById(R.id.logout);

        mDisplayName = mNavigationView.getHeaderView(0).findViewById(R.id.displayName);
        mProfilePicture = mNavigationView.getHeaderView(0).findViewById(R.id.profilePicture);

        Toolbar toolbar = findViewById(R.id.toolbar);
        toolbar.setTitleTextAppearance(this, R.style.HomeActionBarTitleStyle);
        setSupportActionBar(toolbar);

        ActionBar actionbar = getSupportActionBar();
        actionbar.setDisplayHomeAsUpEnabled(true);
        actionbar.setHomeAsUpIndicator(R.drawable.ic_menu);

        mDrawerLayout = findViewById(R.id.drawer_layout);

        Window window = getWindow();
        window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
        window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
        window.setStatusBarColor(ContextCompat.getColor(this, R.color.statusbar_color));

        if (getIntent() != null) {
            if (getIntent().hasExtra(EXTRA_FRAGMENT)) {
                loadFragment(getIntent().getStringExtra(EXTRA_FRAGMENT));
            } else {
                loadFragment(DEFAULT_FRAGMENT);
            }

            if (getIntent().hasExtra(EXTRA_BACK_TO_SCENE)) {
                backToScene = getIntent().getBooleanExtra(EXTRA_BACK_TO_SCENE, false);
            }
        }
    }

    private void loadFragment(String fragment) {
        switch (fragment) {
            case "Login":
                loadLoginFragment();
                break;
            case "Home":
                loadHomeFragment();
                break;
            case "Privacy Policy":
                loadPrivacyPolicyFragment();
                break;
            default:
                Log.e(TAG, "Unknown fragment " + fragment);
        }

    }

    private void loadHomeFragment() {
        Fragment fragment = HomeFragment.newInstance();
        loadFragment(fragment, getString(R.string.home), false);
    }

    private void loadLoginFragment() {
        Fragment fragment = LoginFragment.newInstance();

        loadFragment(fragment, getString(R.string.login), true);
    }

    private void loadPrivacyPolicyFragment() {
        Fragment fragment = PolicyFragment.newInstance();

        loadFragment(fragment, getString(R.string.privacyPolicy), true);
    }

    private void loadFragment(Fragment fragment, String title, boolean addToBackStack) {
        FragmentManager fragmentManager = getFragmentManager();
        FragmentTransaction ft = fragmentManager.beginTransaction();
        ft.replace(R.id.content_frame, fragment);
        if (addToBackStack) {
            ft.addToBackStack(null);
        }
        ft.commit();
        setTitle(title);
        mDrawerLayout.closeDrawer(mNavigationView);
    }

    
    private void updateLoginMenu() {
        if (nativeIsLoggedIn()) {
            mLoginPanel.setVisibility(View.GONE);
            mProfilePanel.setVisibility(View.VISIBLE);
            mLogoutOption.setVisibility(View.VISIBLE);
            updateProfileHeader();
        } else {
            mLoginPanel.setVisibility(View.VISIBLE);
            mProfilePanel.setVisibility(View.GONE);
            mLogoutOption.setVisibility(View.GONE);
            mDisplayName.setText("");
        }
    }

    private void updateProfileHeader() {
        updateProfileHeader(nativeGetDisplayName());
    }
    private void updateProfileHeader(String username) {
        if (!username.isEmpty()) {
            mDisplayName.setText(username);
            updateProfilePicture(username);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        //getMenuInflater().inflate(R.menu.menu_navigation, menu);
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
    public boolean onNavigationItemSelected(@NonNull MenuItem item) {
        switch(item.getItemId()) {
            case R.id.action_home:
                loadHomeFragment();
                return true;
        }
        return false;
    }

    @Override
    protected void onStart() {
        super.onStart();
        updateLoginMenu();
    }

    public void onLoginClicked(View view) {
        loadLoginFragment();
    }

    public void onLogoutClicked(View view) {
        nativeLogout();
        updateLoginMenu();
    }

    public void onSelectedDomain(String domainUrl) {
        goToDomain(domainUrl);
    }

    private void goToLastLocation() {
        goToDomain("");
    }

    private void goToDomain(String domainUrl) {
        Intent intent = new Intent(this, InterfaceActivity.class);
        intent.putExtra(InterfaceActivity.DOMAIN_URL, domainUrl);
        finish();
        intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        startActivity(intent);
    }

    @Override
    public void onLoginCompleted() {
        loadHomeFragment();
        updateLoginMenu();
        if (backToScene) {
            backToScene = false;
            goToLastLocation();
        }
    }

    public void handleUsernameChanged(String username) {
        runOnUiThread(() -> updateProfileHeader(username));
    }

    /**
     * This is a temporary way to get the profile picture URL
     * TODO: this should be get from an API (at the moment there is no one for this)
     */
    private void updateProfilePicture(String username) {
        mProfilePicture.setImageResource(PROFILE_PICTURE_PLACEHOLDER);
        new DownloadProfileImageTask(url ->  { if (url != null && !url.isEmpty()) {
                Picasso.get().load(url).into(mProfilePicture, new RoundProfilePictureCallback());
            } } ).execute(username);
    }

    public void onPrivacyPolicyClicked(View view) {
        loadPrivacyPolicyFragment();
    }

    private class RoundProfilePictureCallback implements Callback {
        @Override
        public void onSuccess() {
            Bitmap imageBitmap = ((BitmapDrawable) mProfilePicture.getDrawable()).getBitmap();
            RoundedBitmapDrawable imageDrawable = RoundedBitmapDrawableFactory.create(getResources(), imageBitmap);
            imageDrawable.setCircular(true);
            imageDrawable.setCornerRadius(Math.max(imageBitmap.getWidth(), imageBitmap.getHeight()) / 2.0f);
            mProfilePicture.setImageDrawable(imageDrawable);
        }

        @Override
        public void onError(Exception e) {
            mProfilePicture.setImageResource(PROFILE_PICTURE_PLACEHOLDER);
        }
    }

    @Override
    public void onBackPressed() {
        int index = getFragmentManager().getBackStackEntryCount() - 1;
        if (index > -1) {
            super.onBackPressed();
            if (backToScene) {
                backToScene = false;
                goToLastLocation();
            }
        } else {
                finishAffinity();
        }
    }

    @Override
    public void onSaveInstanceState(Bundle savedInstanceState) {
        super.onSaveInstanceState(savedInstanceState);
        savedInstanceState.putBoolean(EXTRA_BACK_TO_SCENE, backToScene);
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        backToScene = savedInstanceState.getBoolean(EXTRA_BACK_TO_SCENE, false);
    }
}
