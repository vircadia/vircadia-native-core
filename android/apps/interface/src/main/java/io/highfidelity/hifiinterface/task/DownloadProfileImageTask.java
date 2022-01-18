package io.highfidelity.hifiinterface.task;

import android.os.AsyncTask;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URL;

import io.highfidelity.hifiinterface.HifiUtils;

/**
 * This is a temporary solution until the profile picture URL is
 * available in an API
 */
public class DownloadProfileImageTask extends AsyncTask<String, Void, String> {
    // Note: This should now be available in the API, correct?
    private static final String BASE_PROFILE_URL = "https://metaverse.vircadia.com/live";
    private static final String TAG = "Interface";

    private final DownloadProfileImageResultProcessor mResultProcessor;

    public interface DownloadProfileImageResultProcessor {
        void onResultAvailable(String url);
    }

    public DownloadProfileImageTask(DownloadProfileImageResultProcessor resultProcessor) {
        mResultProcessor = resultProcessor;
    }

    @Override
    protected String doInBackground(String... usernames) {
        URL userPage = null;
        for (String username: usernames) {
            try {
                userPage = new URL(BASE_PROFILE_URL + "/users/" + username);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(
                                userPage.openStream()));

                StringBuffer strBuff = new StringBuffer();
                String inputLine;
                while ((inputLine = in.readLine()) != null) {
                    strBuff.append(inputLine);
                }
                in.close();
                String substr = "img class=\"users-img\" src=\"";
                int indexBegin = strBuff.indexOf(substr) + substr.length();
                if (indexBegin >= substr.length()) {
                    int indexEnd = strBuff.indexOf("\"", indexBegin);
                    if (indexEnd > 0) {
                        String url = strBuff.substring(indexBegin, indexEnd);
                        return HifiUtils.getInstance().absoluteHifiAssetUrl(url, BASE_PROFILE_URL);
                    }
                }
            } catch (IOException e) {
                Log.e(TAG, "Error getting profile picture for username " + username);
            }
        }
        return null;
    }

    @Override
    protected void onPostExecute(String url) {
        super.onPostExecute(url);
        if (mResultProcessor != null) {
            mResultProcessor.onResultAvailable(url);
        }
    }
}
