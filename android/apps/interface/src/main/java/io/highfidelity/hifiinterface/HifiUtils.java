package io.highfidelity.hifiinterface;

import java.net.URI;
import java.net.URISyntaxException;

/**
 * Created by Gabriel Calero & Cristian Duarte on 4/13/18.
 */

public class HifiUtils {

    public static final String METAVERSE_BASE_URL = "https://metaverse.vircadia.com/live";

    private static HifiUtils instance;

    private HifiUtils() {
    }

    public static HifiUtils getInstance() {
        if (instance == null) {
            instance = new HifiUtils();
        }
        return instance;
    }

    public String sanitizeHifiUrl(String urlString) {
        urlString = urlString.trim();
        if (!urlString.isEmpty()) {
            URI uri;
            try {
                uri = new URI(urlString);
            } catch (URISyntaxException e) {
                return urlString;
            }
            if (uri.getScheme() == null || uri.getScheme().isEmpty()) {
                urlString = "hifi://" + urlString;
            }
        }
        return urlString;
    }


    public String absoluteHifiAssetUrl(String urlString) {
        return absoluteHifiAssetUrl(urlString, METAVERSE_BASE_URL);
    }

    public String absoluteHifiAssetUrl(String urlString, String baseUrl) {
        urlString = urlString.trim();
        if (!urlString.isEmpty()) {
            URI uri;
            try {
                uri = new URI(urlString);
            } catch (URISyntaxException e) {
                return urlString;
            }
            if (uri.getScheme() == null || uri.getScheme().isEmpty()) {
                urlString = baseUrl + urlString;
            }
        }
        return urlString;
    }

    public native String getCurrentAddress();

    public native String protocolVersionSignature();

    public native boolean isUserLoggedIn();

    public native void updateHifiSetting(String group, String key, boolean value);
    public native boolean getHifiSettingBoolean(String group, String key, boolean defaultValue);

    public native boolean isKeepingLoggedIn();
}
