package io.highfidelity.hifiinterface.QtPreloader;

import android.content.ComponentName;
import android.content.Context;
import android.content.pm.ComponentInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;

import org.qtproject.qt5.android.bindings.QtApplication;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.util.ArrayList;

import dalvik.system.DexClassLoader;

/**
 * Created by Gabriel Calero & Cristian Duarte on 3/22/18.
 */

public class QtPreloader {

    public String ENVIRONMENT_VARIABLES = "QT_USE_ANDROID_NATIVE_DIALOGS=1";
    private ComponentInfo m_contextInfo;
    private String[] m_qtLibs = null; // required qt libs
    private Context m_context;

    private static final String DEX_PATH_KEY = "dex.path";
    private static final String LIB_PATH_KEY = "lib.path";
    private static final String NATIVE_LIBRARIES_KEY = "native.libraries";
    private static final String ENVIRONMENT_VARIABLES_KEY = "environment.variables";
    private static final String BUNDLED_LIBRARIES_KEY = "bundled.libraries";
    private static final String BUNDLED_IN_LIB_RESOURCE_ID_KEY = "android.app.bundled_in_lib_resource_id";
    private static final String BUNDLED_IN_ASSETS_RESOURCE_ID_KEY = "android.app.bundled_in_assets_resource_id";
    private static final String MAIN_LIBRARY_KEY = "main.library";

    private static final int BUFFER_SIZE = 1024;

    public QtPreloader(Context context) {
        m_context = context;
    }

    public void initQt() {

        try {
            m_contextInfo = m_context.getPackageManager().getActivityInfo(new ComponentName("io.highfidelity.hifiinterface", "io.highfidelity.hifiinterface.InterfaceActivity"),
                    PackageManager.GET_META_DATA);

            if (m_contextInfo.metaData.containsKey("android.app.qt_libs_resource_id")) {
                int resourceId = m_contextInfo.metaData.getInt("android.app.qt_libs_resource_id");
                m_qtLibs = m_context.getResources().getStringArray(resourceId);
            }
            ArrayList<String> libraryList = new ArrayList<>();
            String localPrefix = m_context.getApplicationInfo().dataDir + "/";
            String pluginsPrefix = localPrefix + "qt-reserved-files/";
            cleanOldCacheIfNecessary(localPrefix, pluginsPrefix);
            extractBundledPluginsAndImports(pluginsPrefix);

            for (String lib : m_qtLibs) {
                libraryList.add(localPrefix + "lib/lib" + lib + ".so");
            }

            if (m_contextInfo.metaData.containsKey("android.app.load_local_libs")) {
                String[] extraLibs = m_contextInfo.metaData.getString("android.app.load_local_libs").split(":");
                for (String lib : extraLibs) {
                    if (lib.length() > 0) {
                        if (lib.startsWith("lib/")) {
                            libraryList.add(localPrefix + lib);
                        } else {
                            libraryList.add(pluginsPrefix + lib);
                        }
                    }
                }
            }

            Bundle loaderParams = new Bundle();
            loaderParams.putString(DEX_PATH_KEY, new String());

            loaderParams.putStringArrayList(NATIVE_LIBRARIES_KEY, libraryList);

            loaderParams.putString(ENVIRONMENT_VARIABLES_KEY, ENVIRONMENT_VARIABLES
                    + "\tQML2_IMPORT_PATH=" + pluginsPrefix + "/qml"
                    + "\tQML_IMPORT_PATH=" + pluginsPrefix + "/imports"
                    + "\tQT_PLUGIN_PATH=" + pluginsPrefix + "/plugins");


            // add all bundled Qt libs to loader params
            ArrayList<String> libs = new ArrayList<>();

            String libName = m_contextInfo.metaData.getString("android.app.lib_name");
            loaderParams.putString(MAIN_LIBRARY_KEY, libName); //main library contains main() function
            loaderParams.putStringArrayList(BUNDLED_LIBRARIES_KEY, libs);

            // load and start QtLoader class
            DexClassLoader classLoader = new DexClassLoader(loaderParams.getString(DEX_PATH_KEY), // .jar/.apk files
                    m_context.getDir("outdex", Context.MODE_PRIVATE).getAbsolutePath(), // directory where optimized DEX files should be written.
                    loaderParams.containsKey(LIB_PATH_KEY) ? loaderParams.getString(LIB_PATH_KEY) : null, // libs folder (if exists)
                    m_context.getClassLoader()); // parent loader

            Class<?> loaderClass = classLoader.loadClass(loaderClassName()); // load QtLoader class
            Object qtLoader = loaderClass.newInstance(); // create an instance
            Method prepareAppMethod = qtLoader.getClass().getMethod("loadApplication",
                    contextClassName(),
                    ClassLoader.class,
                    Bundle.class);
            prepareAppMethod.invoke(qtLoader, m_context, classLoader, loaderParams);

            // now load the application library so it's accessible from this class loader
            if (libName != null) {
                System.loadLibrary(libName);
            }
        } catch (Exception e) {
            Log.e(QtApplication.QtTAG, "Error pre-loading HiFi Qt app", e);
        }
    }

    protected String loaderClassName() {
        return "org.qtproject.qt5.android.QtActivityDelegate";
    }

    protected Class<?> contextClassName() {
        return android.app.Activity.class;
    }


    private void deleteRecursively(File directory) {
        File[] files = directory.listFiles();
        if (files != null) {
            for (File file : files) {
                if (file.isDirectory()) {
                    deleteRecursively(file);
                } else {
                    file.delete();
                }
            }

            directory.delete();
        }
    }

    private void cleanOldCacheIfNecessary(String oldLocalPrefix, String localPrefix) {
        File newCache = new File(localPrefix);
        if (!newCache.exists()) {
            {
                File oldPluginsCache = new File(oldLocalPrefix + "plugins/");
                if (oldPluginsCache.exists() && oldPluginsCache.isDirectory()) {
                    deleteRecursively(oldPluginsCache);
                }
            }

            {
                File oldImportsCache = new File(oldLocalPrefix + "imports/");
                if (oldImportsCache.exists() && oldImportsCache.isDirectory()) {
                    deleteRecursively(oldImportsCache);
                }
            }

            {
                File oldQmlCache = new File(oldLocalPrefix + "qml/");
                if (oldQmlCache.exists() && oldQmlCache.isDirectory()) {
                    deleteRecursively(oldQmlCache);
                }
            }
        }
    }

    static private void copyFile(InputStream inputStream, OutputStream outputStream)
            throws IOException {
        byte[] buffer = new byte[BUFFER_SIZE];

        int count;
        while ((count = inputStream.read(buffer)) > 0) {
            outputStream.write(buffer, 0, count);
        }
    }

    private void copyAsset(String source, String destination)
            throws IOException {
        // Already exists, we don't have to do anything
        File destinationFile = new File(destination);
        if (destinationFile.exists()) {
            return;
        }

        File parentDirectory = destinationFile.getParentFile();
        if (!parentDirectory.exists()) {
            parentDirectory.mkdirs();
        }

        destinationFile.createNewFile();

        AssetManager assetsManager = m_context.getAssets();
        InputStream inputStream = assetsManager.open(source);
        OutputStream outputStream = new FileOutputStream(destinationFile);
        copyFile(inputStream, outputStream);

        inputStream.close();
        outputStream.close();
    }

    private static void createBundledBinary(String source, String destination)
            throws IOException {
        // Already exists, we don't have to do anything
        File destinationFile = new File(destination);
        if (destinationFile.exists()) {
            return;
        }

        File parentDirectory = destinationFile.getParentFile();
        if (!parentDirectory.exists()) {
            parentDirectory.mkdirs();
        }

        destinationFile.createNewFile();

        InputStream inputStream = new FileInputStream(source);
        OutputStream outputStream = new FileOutputStream(destinationFile);
        copyFile(inputStream, outputStream);

        inputStream.close();
        outputStream.close();
    }

    private boolean cleanCacheIfNecessary(String pluginsPrefix, long packageVersion) {
        File versionFile = new File(pluginsPrefix + "cache.version");

        long cacheVersion = 0;
        if (versionFile.exists() && versionFile.canRead()) {
            try {
                DataInputStream inputStream = new DataInputStream(new FileInputStream(versionFile));
                cacheVersion = inputStream.readLong();
                inputStream.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        if (cacheVersion != packageVersion) {
            deleteRecursively(new File(pluginsPrefix));
            return true;
        } else {
            return false;
        }
    }

    private void extractBundledPluginsAndImports(String pluginsPrefix) throws IOException {
        String libsDir = m_context.getApplicationInfo().nativeLibraryDir + "/";
        long packageVersion = -1;
        try {
            PackageInfo packageInfo = m_context.getPackageManager().getPackageInfo(m_context.getPackageName(), 0);
            packageVersion = packageInfo.lastUpdateTime;
        } catch (Exception e) {
            e.printStackTrace();
        }


        if (!cleanCacheIfNecessary(pluginsPrefix, packageVersion)) {
            return;
        }

        {
            File versionFile = new File(pluginsPrefix + "cache.version");

            File parentDirectory = versionFile.getParentFile();
            if (!parentDirectory.exists()) {
                parentDirectory.mkdirs();
            }

            versionFile.createNewFile();

            DataOutputStream outputStream = new DataOutputStream(new FileOutputStream(versionFile));
            outputStream.writeLong(packageVersion);
            outputStream.close();
        }

        {
            String key = BUNDLED_IN_LIB_RESOURCE_ID_KEY;
            if (m_contextInfo.metaData.containsKey(key)) {
                String[] list = m_context.getResources().getStringArray(m_contextInfo.metaData.getInt(key));

                for (String bundledImportBinary : list) {
                    String[] split = bundledImportBinary.split(":");
                    String sourceFileName = libsDir + split[0];
                    String destinationFileName = pluginsPrefix + split[1];
                    createBundledBinary(sourceFileName, destinationFileName);
                }
            }
        }

        {
            String key = BUNDLED_IN_ASSETS_RESOURCE_ID_KEY;
            if (m_contextInfo.metaData.containsKey(key)) {
                String[] list = m_context.getResources().getStringArray(m_contextInfo.metaData.getInt(key));

                for (String fileName : list) {
                    String[] split = fileName.split(":");
                    String sourceFileName = split[0];
                    String destinationFileName = pluginsPrefix + split[1];
                    copyAsset(sourceFileName, destinationFileName);
                }
            }

        }
    }
}
