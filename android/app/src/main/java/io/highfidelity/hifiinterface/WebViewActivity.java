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

//package hifiinterface.highfidelity.io.mybrowserapplication;
package io.highfidelity.hifiinterface;

import android.app.ActionBar;
import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.net.http.SslError;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.webkit.SslErrorHandler;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceResponse;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.ProgressBar;
import android.widget.Toast;
import android.widget.Toolbar;
import android.os.Looper;
import java.lang.Thread;
import java.lang.Runnable;

import java.net.MalformedURLException;
import java.net.URL;

public class WebViewActivity extends Activity {

    public static final String WEB_VIEW_ACTIVITY_EXTRA_URL = "url";

    private native void nativeProcessURL(String url);

    private WebView myWebView;
    private ProgressBar mProgressBar;
    private ActionBar mActionBar;
    private String mUrl;

    enum SafenessLevel {
        NOT_ANALYZED_YET(""),
        NOT_SECURE(""),
        SECURE("\uD83D\uDD12 "),
        BAD_SECURE("\uD83D\uDD13 ");

        String icon;
        SafenessLevel(String icon) {
            this.icon = icon;
        }
    }

    private SafenessLevel safenessLevel = SafenessLevel.NOT_ANALYZED_YET;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_web_view);

        setActionBar((Toolbar) findViewById(R.id.toolbar_actionbar));
        mActionBar = getActionBar();
        mActionBar.setDisplayHomeAsUpEnabled(true);

        mProgressBar = (ProgressBar) findViewById(R.id.toolbarProgressBar);

        mUrl = getIntent().getStringExtra(WEB_VIEW_ACTIVITY_EXTRA_URL);
        myWebView = (WebView) findViewById(R.id.web_view);
        myWebView.setWebViewClient(new HiFiWebViewClient());
        myWebView.setWebChromeClient(new HiFiWebChromeClient());
        WebSettings webSettings = myWebView.getSettings();
        webSettings.setJavaScriptEnabled(true);
        webSettings.setBuiltInZoomControls(true);
        webSettings.setDisplayZoomControls(false);
        myWebView.loadUrl(mUrl);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // Check if the key event was the Back button and if there's history
        if ((keyCode == KeyEvent.KEYCODE_BACK) && myWebView.canGoBack()) {
            myWebView.goBack();
            return true;
        }
        // If it wasn't the Back key or there's no web page history, bubble up to the default
        // system behavior (probably exit the activity)
        return super.onKeyDown(keyCode, event);
    }

    private void showSubtitleWithUrl(String url) {
        try {
            mActionBar.setSubtitle(safenessLevel.icon + new URL(url.toString()).getHost());
        } catch (MalformedURLException e) {
            Toast.makeText(WebViewActivity.this, "Error loading page: " + "bad url", Toast.LENGTH_LONG).show();
            Log.e("openUrl", "bad url");
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.web_view_menu, menu);
        return true;
    }

    private String intentUrlOrWebUrl() {
        return myWebView==null || myWebView.getUrl()==null?mUrl:myWebView.getUrl();
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

    class HiFiWebViewClient extends WebViewClient {

        @Override
        public void onPageFinished(WebView view, String url) {
            super.onPageFinished(view, url);
            mProgressBar.setVisibility(View.GONE);
            if (safenessLevel!=SafenessLevel.BAD_SECURE) {
                if (url.startsWith("https:")) {
                    safenessLevel=SafenessLevel.SECURE;
                } else {
                    safenessLevel=SafenessLevel.NOT_SECURE;
                }
            }
            showSubtitleWithUrl(url);
        }

        @Override
        public void onPageStarted(WebView view, String url, Bitmap favicon) {
            super.onPageStarted(view, url, favicon);
            safenessLevel = SafenessLevel.NOT_ANALYZED_YET;
            mProgressBar.setVisibility(View.VISIBLE);
            mProgressBar.setProgress(0);
            showSubtitleWithUrl(url);
        }

        @Override
        public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error) {
            Toast.makeText(WebViewActivity.this, "Error loading page: " + error.getDescription(), Toast.LENGTH_LONG).show();
            if (ERROR_FAILED_SSL_HANDSHAKE == error.getErrorCode()) {
                safenessLevel = SafenessLevel.BAD_SECURE;
            }
        }

        @Override
        public void onReceivedHttpError(WebView view, WebResourceRequest request, WebResourceResponse errorResponse) {
            Toast.makeText(WebViewActivity.this, "Network Error loading page: " + errorResponse.getReasonPhrase(), Toast.LENGTH_LONG).show();
        }

        @Override
        public void onReceivedSslError(WebView view, SslErrorHandler handler, SslError error) {
            super.onReceivedSslError(view, handler, error);
            Toast.makeText(WebViewActivity.this, "SSL error loading page: " + error.toString(), Toast.LENGTH_LONG).show();
            safenessLevel = SafenessLevel.BAD_SECURE;
        }

        private boolean isFst(WebResourceRequest request) {
            return isFst(request.getUrl().toString());
        }

        private boolean isFst(String url) {
            return url.endsWith(".fst");
        }

        @Override
        public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
            // managing avatar selections
            if (isFst(request)) {
                final String url = request.getUrl().toString();
                new Thread(new Runnable() {
                    public void run() {
                        nativeProcessURL(url);
                    }
                }).start(); // Avoid deadlock in Qt dialog
                WebViewActivity.this.finish();
                return true;
            }
            return super.shouldOverrideUrlLoading(view, request);
        }

        @Override
        public void onLoadResource(WebView view, String url) {
            if (isFst(url)) {
                // processed separately
            } else {
                super.onLoadResource(view, url);
            }
        }
    }

    class HiFiWebChromeClient extends WebChromeClient {

        @Override
        public void onProgressChanged(WebView view, int newProgress) {
            super.onProgressChanged(view, newProgress);
            mProgressBar.setProgress(newProgress);
        }

        @Override
        public void onReceivedTitle(WebView view, String title) {
            super.onReceivedTitle(view, title);
            mActionBar.setTitle(title);
        }

    }
}
