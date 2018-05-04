package io.highfidelity.hifiinterface;

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

    public native String getCurrentAddress();

}
