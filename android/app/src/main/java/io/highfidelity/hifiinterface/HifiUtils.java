package io.highfidelity.hifiinterface;

import java.net.URI;
import java.net.URISyntaxException;

/**
 * Created by Gabriel Calero & Cristian Duarte on 4/13/18.
 */

public class HifiUtils {

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

    public native String getCurrentAddress();

    public native String protocolVersionSignature();

}
