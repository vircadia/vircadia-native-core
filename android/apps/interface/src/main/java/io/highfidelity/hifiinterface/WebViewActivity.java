//
//  WebViewActivity.java
//  interface/java
//
//  Created by Cristian Duarte and Gabriel Calero on 8/17/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
package io.highfidelity.hifiinterface;

import android.app.ActionBar;
import android.app.Activity;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.Toast;

import java.net.MalformedURLException;
import java.net.URL;

import io.highfidelity.hifiinterface.fragment.WebViewFragment;
import io.highfidelity.hifiinterface.fragment.WebViewFragment.OnWebViewInteractionListener;

public class WebViewActivity extends Activity implements OnWebViewInteractionListener {

    public static final String WEB_VIEW_ACTIVITY_EXTRA_URL = "url";
    public static final String WEB_VIEW_ACTIVITY_EXTRA_CLEAR_COOKIES = "clear_cookies";
    public static final String RESULT_OAUTH_CODE = "code";
    public static final String RESULT_OAUTH_STATE = "state";

    private static final String FRAGMENT_TAG = "WebViewActivity_WebFragment";
    private static final String OAUTH_CODE = "code";
    private static final String OAUTH_STATE = "state";

    private native void nativeProcessURL(String url);

    private ActionBar mActionBar;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_web_view);

        setActionBar(findViewById(R.id.toolbar_actionbar));
        mActionBar = getActionBar();
        mActionBar.setDisplayHomeAsUpEnabled(true);

        loadWebViewFragment(getIntent().getStringExtra(WEB_VIEW_ACTIVITY_EXTRA_URL), getIntent().getBooleanExtra(WEB_VIEW_ACTIVITY_EXTRA_CLEAR_COOKIES, false));
    }

    private void loadWebViewFragment(String url, boolean clearCookies) {
        WebViewFragment fragment = WebViewFragment.newInstance();
        Bundle bundle = new Bundle();
        bundle.putString(WebViewFragment.URL, url);
        bundle.putBoolean(WebViewFragment.TOOLBAR_VISIBLE, false);
        bundle.putBoolean(WebViewFragment.CLEAR_COOKIES, clearCookies);
        fragment.setArguments(bundle);
        FragmentManager fragmentManager = getFragmentManager();
        FragmentTransaction ft = fragmentManager.beginTransaction();
        ft.replace(R.id.content_frame, fragment, FRAGMENT_TAG);
        ft.commit();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        WebViewFragment fragment = (WebViewFragment) getFragmentManager().findFragmentByTag(FRAGMENT_TAG);
        if (fragment != null && fragment.onKeyDown(keyCode)) {
            return true;
        }
        // If it wasn't the Back key or there's no web page history, bubble up to the default
        // system behavior (probably exit the activity)
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.web_view_menu, menu);
        return true;
    }

    private String intentUrlOrWebUrl() {
        return ((WebViewFragment) getFragmentManager().findFragmentById(R.id.content_frame)).intentUrlOrWebUrl();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item){
        switch (item.getItemId()) {
            // Respond to the action bar's Up/Home/back button
            case android.R.id.home:
                finish();
                break;
            case R.id.action_browser:
                Intent i = new Intent(Intent.ACTION_VIEW);
                i.setData(Uri.parse(intentUrlOrWebUrl()));
                startActivity(i);
                return true;
            case R.id.action_share:
                Intent is = new Intent(Intent.ACTION_SEND);
                is.setType("text/plain");
                is.putExtra(Intent.EXTRA_SUBJECT, getString(R.string.web_view_action_share_subject));
                is.putExtra(Intent.EXTRA_TEXT, intentUrlOrWebUrl());
                startActivity(Intent.createChooser(is, getString(R.string.web_view_action_share_chooser)));
                return true;
        }
        return super.onOptionsItemSelected(item);
    }


    @Override
    public void processURL(String url) {
        nativeProcessURL(url);
    }

    @Override
    public void onWebLoaded(String url, WebViewFragment.SafenessLevel safenessLevel) {
        try {
            mActionBar.setSubtitle(safenessLevel.icon + new URL(url.toString()).getHost());
        } catch (MalformedURLException e) {
            Toast.makeText(WebViewActivity.this, "Error loading page: " + "bad url", Toast.LENGTH_LONG).show();
            Log.e("openUrl", "bad url");
        }
    }

    @Override
    public void onTitleReceived(String title) {
        mActionBar.setTitle(title);
    }

    @Override
    public void onExpand() { }

    @Override
    public void onOAuthAuthorizeCallback(Uri uri) {
        Intent result = new Intent();
        result.putExtra(RESULT_OAUTH_CODE, uri.getQueryParameter(OAUTH_CODE));
        result.putExtra(RESULT_OAUTH_STATE, uri.getQueryParameter(OAUTH_STATE));
        setResult(Activity.RESULT_OK, result);
        finish();
    }

}
