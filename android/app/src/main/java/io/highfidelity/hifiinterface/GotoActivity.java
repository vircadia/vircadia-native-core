package io.highfidelity.hifiinterface;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.AppCompatButton;
import android.support.v7.widget.Toolbar;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;

import java.net.URI;
import java.net.URISyntaxException;

public class GotoActivity extends AppCompatActivity {

    public static final String PARAM_DOMAIN_URL = "domain_url";

    private EditText mUrlEditText;
    private AppCompatButton mGoBtn;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_goto);

        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        toolbar.setTitleTextAppearance(this, R.style.HomeActionBarTitleStyle);
        setSupportActionBar(toolbar);

        ActionBar actionbar = getSupportActionBar();
        actionbar.setDisplayHomeAsUpEnabled(true);

        mUrlEditText = (EditText) findViewById(R.id.url_text);
        mUrlEditText.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View view, int i, KeyEvent keyEvent) {
                if (i == KeyEvent.KEYCODE_ENTER) {
                    actionGo();
                    return true;
                }
                return false;
            }
        });

        mUrlEditText.setText(HifiUtils.getInstance().getCurrentAddress());

        mGoBtn = (AppCompatButton) findViewById(R.id.go_btn);

        mGoBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                actionGo();
            }
        });
    }

    private void actionGo() {
        String urlString = mUrlEditText.getText().toString();
        if (!urlString.trim().isEmpty()) {
            URI uri;
            try {
                uri = new URI(urlString);
            } catch (URISyntaxException e) {
                return;
            }
            if (uri.getScheme() == null || uri.getScheme().isEmpty()) {
                urlString = "hifi://" + urlString;
            }

            Intent intent = new Intent();
            intent.putExtra(GotoActivity.PARAM_DOMAIN_URL, urlString);
            setResult(RESULT_OK, intent);
            finish();
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        switch (id) {
            case android.R.id.home:
                finish();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }
}
