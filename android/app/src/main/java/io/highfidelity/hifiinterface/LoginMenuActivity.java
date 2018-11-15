package io.highfidelity.hifiinterface;


import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import io.highfidelity.hifiinterface.fragment.LoginFragment;
import io.highfidelity.hifiinterface.fragment.OnBackPressedListener;
import io.highfidelity.hifiinterface.fragment.SignupFragment;
import io.highfidelity.hifiinterface.fragment.StartMenuFragment;

public class LoginMenuActivity extends AppCompatActivity
                                implements StartMenuFragment.StartMenuInteractionListener,
                                                LoginFragment.OnLoginInteractionListener,
                                                SignupFragment.OnSignupInteractionListener {

    /**
     * Set EXTRA_FINISH_ON_BACK to finish the app when back button is pressed
     */
    public static final String EXTRA_FINISH_ON_BACK = "finishOnBack";

    /**
     * Set EXTRA_BACK_TO_SCENE to back to the scene
     */
    public static final String EXTRA_BACK_TO_SCENE = "backToScene";

    /**
     * Set EXTRA_BACK_ON_SKIP to finish this activity when skip button is pressed
     */
    public static final String EXTRA_BACK_ON_SKIP = "backOnSkip";

    public static final String EXTRA_DOMAIN_URL = "url";

    private boolean finishOnBack;
    private boolean backToScene;
    private boolean backOnSkip;
    private String domainUrlToBack;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_encourage_login);

        finishOnBack = getIntent().getBooleanExtra(EXTRA_FINISH_ON_BACK, false);
        backToScene = getIntent().getBooleanExtra(EXTRA_BACK_TO_SCENE, false);
        domainUrlToBack =  getIntent().getStringExtra(EXTRA_DOMAIN_URL);
        backOnSkip = getIntent().getBooleanExtra(EXTRA_BACK_ON_SKIP, false);

        if (savedInstanceState != null) {
            finishOnBack = savedInstanceState.getBoolean(EXTRA_FINISH_ON_BACK, false);
            backToScene = savedInstanceState.getBoolean(EXTRA_BACK_TO_SCENE, false);
            backOnSkip = savedInstanceState.getBoolean(EXTRA_BACK_ON_SKIP, false);
            domainUrlToBack = savedInstanceState.getString(EXTRA_DOMAIN_URL);
        }

        loadMenuFragment();
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean(EXTRA_FINISH_ON_BACK, finishOnBack);
        outState.putBoolean(EXTRA_BACK_TO_SCENE, backToScene);
        outState.putString(EXTRA_DOMAIN_URL, domainUrlToBack);
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        finishOnBack = savedInstanceState.getBoolean(EXTRA_FINISH_ON_BACK, false);
        backToScene = savedInstanceState.getBoolean(EXTRA_BACK_TO_SCENE, false);
        backOnSkip = savedInstanceState.getBoolean(EXTRA_BACK_ON_SKIP, false);
        domainUrlToBack = savedInstanceState.getString(EXTRA_DOMAIN_URL);
    }

    private void loadMenuFragment() {
        FragmentManager fragmentManager = getFragmentManager();
        FragmentTransaction fragmentTransaction = fragmentManager.beginTransaction();
        Fragment fragment = StartMenuFragment.newInstance();
        fragmentTransaction.replace(R.id.content_frame, fragment);
        fragmentTransaction.addToBackStack(fragment.toString());
        fragmentTransaction.setTransition(FragmentTransaction.TRANSIT_FRAGMENT_OPEN);
        fragmentTransaction.commit();
        hideStatusBar();
    }

    @Override
    protected void onResume() {
        super.onResume();
        hideStatusBar();
    }

    private void hideStatusBar() {
        View decorView = getWindow().getDecorView();
        // Hide the status bar.
        int uiOptions = View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN;
        decorView.setSystemUiVisibility(uiOptions);
    }

    @Override
    public void onSignupButtonClicked() {
        loadSignupFragment();
    }

    @Override
    public void onLoginButtonClicked() {
        loadLoginFragment(false);
    }

    @Override
    public void onSkipLoginClicked() {
        if (backOnSkip) {
            onBackPressed();
        } else {
            loadMainActivity();
        }
    }

    @Override
    public void onSteamLoginButtonClicked() {
        loadLoginFragment(true);
    }

    private void loadSignupFragment() {
        FragmentManager fragmentManager = getFragmentManager();
        FragmentTransaction fragmentTransaction = fragmentManager.beginTransaction();
        Fragment fragment = SignupFragment.newInstance();
        String tag = getString(R.string.tagFragmentSignup);
        fragmentTransaction.replace(R.id.content_frame, fragment, tag);
        fragmentTransaction.addToBackStack(tag);
        fragmentTransaction.setTransition(FragmentTransaction.TRANSIT_FRAGMENT_OPEN);
        fragmentTransaction.commit();
        hideStatusBar();
    }

    private void loadLoginFragment(boolean useOauth) {
        FragmentManager fragmentManager = getFragmentManager();
        FragmentTransaction fragmentTransaction = fragmentManager.beginTransaction();
        Fragment fragment = LoginFragment.newInstance(useOauth);
        String tag = getString(R.string.tagFragmentLogin);
        fragmentTransaction.replace(R.id.content_frame, fragment, tag);
        fragmentTransaction.addToBackStack(tag);
        fragmentTransaction.setTransition(FragmentTransaction.TRANSIT_FRAGMENT_OPEN);
        fragmentTransaction.commit();
        hideStatusBar();
    }

    @Override
    public void onLoginCompleted() {
        loadMainActivity();
    }

    @Override
    public void onCancelLogin() {
        getFragmentManager().popBackStack();
    }

    @Override
    public void onCancelSignup() {
        getFragmentManager().popBackStack();
    }

    private void loadMainActivity() {
        finish();
        if (backToScene) {
            backToScene = false;
            goToDomain(domainUrlToBack != null? domainUrlToBack : "");
        } else {
            startActivity(new Intent(this, MainActivity.class));
        }
    }

    private void goToDomain(String domainUrl) {
        Intent intent = new Intent(this, InterfaceActivity.class);
        intent.putExtra(InterfaceActivity.DOMAIN_URL, domainUrl);
        finish();
        intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        startActivity(intent);
    }


    @Override
    public void onSignupCompleted() {
        loadMainActivity();
    }

    @Override
    public void onBackPressed() {
        FragmentManager fm = getFragmentManager();
        int index = fm.getBackStackEntryCount() - 1;
        if (index > 0) {
            FragmentManager.BackStackEntry backEntry = fm.getBackStackEntryAt(index);
            String tag = backEntry.getName();
            Fragment topFragment = getFragmentManager().findFragmentByTag(tag);
            if (!(topFragment instanceof OnBackPressedListener) ||
                    !((OnBackPressedListener) topFragment).doBack()) {
                super.onBackPressed();
            }
        } else if (finishOnBack){
            finishAffinity();
        } else {
            finish();
        }
    }
}
