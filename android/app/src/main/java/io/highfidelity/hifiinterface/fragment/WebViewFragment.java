package io.highfidelity.hifiinterface.fragment;

import android.app.Fragment;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.net.http.SslError;
import android.os.Bundle;
import android.os.Handler;
import android.text.TextUtils;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AlphaAnimation;
import android.webkit.CookieManager;
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

import io.highfidelity.hifiinterface.BuildConfig;
import io.highfidelity.hifiinterface.R;
import io.highfidelity.hifiinterface.WebViewActivity;

public class WebViewFragment extends Fragment implements GestureDetector.OnGestureListener {

    public static final String URL = "url";
    public static final String TOOLBAR_VISIBLE = "toolbar_visible";
    public static final String CLEAR_COOKIES = "clear_cookies";
    private static final long DELAY_HIDE_TOOLBAR_MILLIS = 3000;
    private static final long FADE_OUT_DURATION = 2000;

    private WebView myWebView;
    private GestureDetector gestureDetector;
    private View mToolbar;
    private ProgressBar mProgressBar;
    private String mUrl;
    private boolean mToolbarVisible;
    private boolean mClearCookies;

    private OnWebViewInteractionListener mListener;
    private Runnable mCloseAction;
    private Handler mHandler;

    private Runnable mHideToolbar = new Runnable() {
        @Override
        public void run() {
            if (mToolbar != null) {
                AlphaAnimation anim = new AlphaAnimation(1.0f, 0.0f);
                anim.setDuration(FADE_OUT_DURATION);
                anim.setFillAfter(true);
                mToolbar.startAnimation(anim);
            }
        }
    };

    public boolean onKeyDown(int keyCode) {
        // Check if the key event was the Back button and if there's history
        if ((keyCode == KeyEvent.KEYCODE_BACK) && myWebView.canGoBack()) {
            myWebView.goBack();
            return true;
        }
        return false;
    }

    public String intentUrlOrWebUrl() {
        return myWebView == null || myWebView.getUrl() == null ? mUrl : myWebView.getUrl();
    }

    public void loadUrl(String url, boolean showToolbar) {
        mUrl = url;
        mToolbarVisible = showToolbar;
        loadUrl(myWebView, url);
    }

    private void loadUrl(WebView webView, String url) {
        webView.setVisibility(View.GONE);
        webView.getSettings().setLoadWithOverviewMode(true);
        webView.getSettings().setUseWideViewPort(true);
        webView.loadUrl(url);
        mToolbar.setVisibility(mToolbarVisible ? View.VISIBLE : View.GONE);
        mToolbar.clearAnimation();
        if (mToolbarVisible) {
            mHandler.postDelayed(mHideToolbar, DELAY_HIDE_TOOLBAR_MILLIS);
        }
    }

    public void setToolbarVisible(boolean visible) {
        mToolbar.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    public void setCloseAction(Runnable closeAction) {
        this.mCloseAction = closeAction;
    }

    @Override
    public boolean onDown(MotionEvent motionEvent) {
        mHandler.removeCallbacks(mHideToolbar);
        if (mToolbarVisible) {
            mToolbar.setVisibility(mToolbarVisible ? View.VISIBLE : View.GONE);
            mToolbar.clearAnimation();
            mHandler.postDelayed(mHideToolbar, DELAY_HIDE_TOOLBAR_MILLIS);
        }
        return false;
    }

    @Override
    public void onShowPress(MotionEvent motionEvent) {

    }

    @Override
    public boolean onSingleTapUp(MotionEvent motionEvent) {
        return false;
    }

    @Override
    public boolean onScroll(MotionEvent motionEvent, MotionEvent motionEvent1, float v, float v1) {
        return false;
    }

    @Override
    public void onLongPress(MotionEvent motionEvent) {

    }

    @Override
    public boolean onFling(MotionEvent motionEvent, MotionEvent motionEvent1, float v, float v1) {
        return false;
    }

    public void close() {
        myWebView.loadUrl("about:blank");
        if (mCloseAction != null) {
            mCloseAction.run();
        }
    }

    public enum SafenessLevel {
        NOT_ANALYZED_YET(""),
        NOT_SECURE(""),
        SECURE("\uD83D\uDD12 "),
        BAD_SECURE("\uD83D\uDD13 ");

        public String icon;
        SafenessLevel(String icon) {
            this.icon = icon;
        }
    }

    private SafenessLevel safenessLevel = SafenessLevel.NOT_ANALYZED_YET;


    public WebViewFragment() {
        // Required empty public constructor
    }

    public static WebViewFragment newInstance() {
        WebViewFragment fragment = new WebViewFragment();
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (getArguments() != null) {
            mUrl = getArguments().getString(URL);
            mToolbarVisible = getArguments().getBoolean(TOOLBAR_VISIBLE);
            mClearCookies = getArguments().getBoolean(CLEAR_COOKIES);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_web_view, container, false);
        mProgressBar = rootView.findViewById(R.id.toolbarProgressBar);
        myWebView = rootView.findViewById(R.id.web_view);
        if (mClearCookies) {
            CookieManager.getInstance().removeAllCookies(null);
        }

        mHandler = new Handler();
        gestureDetector = new GestureDetector(this);
        gestureDetector.setOnDoubleTapListener(new GestureDetector.OnDoubleTapListener() {
            @Override
            public boolean onSingleTapConfirmed(MotionEvent motionEvent) {
                return false;
            }

            @Override
            public boolean onDoubleTap(MotionEvent motionEvent) {
                expand();
                return false;
            }

            @Override
            public boolean onDoubleTapEvent(MotionEvent motionEvent) {
                return false;
            }
        });
        myWebView.setOnTouchListener((v, event) -> gestureDetector.onTouchEvent(event));

        myWebView.setWebViewClient(new HiFiWebViewClient());
        myWebView.setWebChromeClient(new HiFiWebChromeClient());
        WebSettings webSettings = myWebView.getSettings();
        webSettings.setJavaScriptEnabled(true);
        webSettings.setBuiltInZoomControls(true);
        webSettings.setDisplayZoomControls(false);

        mToolbar = rootView.findViewById(R.id.toolbar);
        mToolbar.findViewById(R.id.viewFullScreen).setOnClickListener(view -> {
            expand();
        });
        mToolbar.findViewById(R.id.close).setOnClickListener(view -> {
            close();
        });
        if (mUrl != null) {
            loadUrl(myWebView, mUrl);
        }
        return rootView;
    }

    private void expand() {
        if (mListener != null) {
            mListener.onExpand();
        }
        Intent intent = new Intent(getActivity(), WebViewActivity.class);
        intent.putExtra(WebViewActivity.WEB_VIEW_ACTIVITY_EXTRA_URL, intentUrlOrWebUrl());
        getActivity().startActivity(intent);
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof OnWebViewInteractionListener) {
            mListener = (OnWebViewInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnWebViewInteractionListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

    public interface OnWebViewInteractionListener {
        void processURL(String url);
        void onWebLoaded(String url, SafenessLevel safenessLevel);
        void onTitleReceived(String title);
        void onExpand();
        void onOAuthAuthorizeCallback(Uri uri);
    }


    class HiFiWebViewClient extends WebViewClient {
        @Override
        public void onPageFinished(WebView view, String url) {
            super.onPageFinished(view, url);
            mProgressBar.setVisibility(View.GONE);
            if (safenessLevel!= SafenessLevel.BAD_SECURE) {
                if (url.startsWith("https:")) {
                    safenessLevel = SafenessLevel.SECURE;
                } else {
                    safenessLevel = SafenessLevel.NOT_SECURE;
                }
            }
            if (mListener != null) {
                myWebView.setVisibility(View.VISIBLE);
                mListener.onWebLoaded(url, safenessLevel);
            }
        }

        @Override
        public void onPageStarted(WebView view, String url, Bitmap favicon) {
            super.onPageStarted(view, url, favicon);
            safenessLevel = SafenessLevel.NOT_ANALYZED_YET;
            mProgressBar.setVisibility(View.VISIBLE);
            mProgressBar.setProgress(0);
            if (mListener != null) {
                myWebView.setVisibility(View.VISIBLE);
                mListener.onWebLoaded(url, safenessLevel);
            }
        }

        @Override
        public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error) {
            Toast.makeText(getActivity(), "Error loading page: " + error.getDescription(), Toast.LENGTH_LONG).show();
            if (ERROR_FAILED_SSL_HANDSHAKE == error.getErrorCode()) {
                safenessLevel = SafenessLevel.BAD_SECURE;
            }
        }

        @Override
        public void onReceivedHttpError(WebView view, WebResourceRequest request, WebResourceResponse errorResponse) {
            Toast.makeText(getActivity(), "Network Error loading page: " + errorResponse.getReasonPhrase(), Toast.LENGTH_LONG).show();
        }

        @Override
        public void onReceivedSslError(WebView view, SslErrorHandler handler, SslError error) {
            super.onReceivedSslError(view, handler, error);
            Toast.makeText(getActivity(), "SSL error loading page: " + error.toString(), Toast.LENGTH_LONG).show();
            safenessLevel = SafenessLevel.BAD_SECURE;
        }

        private boolean isFst(WebResourceRequest request) {
            return isFst(request.getUrl().toString());
        }

        private boolean isFst(String url) {
            return url.contains(".fst");
        }

        @Override
        public void onLoadResource(WebView view, String url) {
            if (isFst(url)) {
                // processed separately
            } else {
                super.onLoadResource(view, url);
            }
        }

        @Override
        public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
            if (!TextUtils.isEmpty(BuildConfig.OAUTH_REDIRECT_URI) &&
                    request.getUrl().toString().startsWith(BuildConfig.OAUTH_REDIRECT_URI)) {
                if (mListener != null) {
                    mListener.onOAuthAuthorizeCallback(request.getUrl());
                }
                return true;
            }
            return super.shouldOverrideUrlLoading(view, request);
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
            if (mListener != null) {
                mListener.onTitleReceived(title);
            }
        }

    }

}
