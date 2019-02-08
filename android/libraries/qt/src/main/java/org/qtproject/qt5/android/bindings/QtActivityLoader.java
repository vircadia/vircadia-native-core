/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
    Contact: http://www.qt-project.org/legal

    Commercial License Usage
    Licensees holding valid commercial Qt licenses may use this file in
    accordance with the commercial license agreement provided with the
    Software or, alternatively, in accordance with the terms contained in
    a written agreement between you and Digia.  For licensing terms and
    conditions see http://qt.digia.com/licensing.  For further information
    use the contact form at http://qt.digia.com/contact-us.

    BSD License Usage
    Alternatively, this file may be used under the BSD license as follows:
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package org.qtproject.qt5.android.bindings;

import android.app.AlertDialog;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ComponentInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Window;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;

import dalvik.system.DexClassLoader;

public class QtActivityLoader {
    private static final String DEX_PATH_KEY = "dex.path";
    private static final String LIB_PATH_KEY = "lib.path";
    private static final String NATIVE_LIBRARIES_KEY = "native.libraries";
    private static final String ENVIRONMENT_VARIABLES_KEY = "environment.variables";
    private static final String APPLICATION_PARAMETERS_KEY = "application.parameters";
    private static final String BUNDLED_LIBRARIES_KEY = "bundled.libraries";
    private static final String BUNDLED_IN_LIB_RESOURCE_ID_KEY = "android.app.bundled_in_lib_resource_id";
    private static final String BUNDLED_IN_ASSETS_RESOURCE_ID_KEY = "android.app.bundled_in_assets_resource_id";
    private static final String MAIN_LIBRARY_KEY = "main.library";
    private static final String STATIC_INIT_CLASSES_KEY = "static.init.classes";
    private static final String EXTRACT_STYLE_KEY = "extract.android.style";
    private static final String EXTRACT_STYLE_MINIMAL_KEY = "extract.android.style.option";
    private static final int BUFFER_SIZE = 1024;

    String APPLICATION_PARAMETERS = null; // use this variable to pass any parameters to your application,
    String ENVIRONMENT_VARIABLES = "QT_USE_ANDROID_NATIVE_DIALOGS=1";
    String[] QT_ANDROID_THEMES = null;
    String QT_ANDROID_DEFAULT_THEME = null;

    private String[] m_qtLibs = null; // required qt libs
    private int m_displayDensity = -1;
    private ContextWrapper m_context;
    private ComponentInfo m_contextInfo;
    private Class<?> m_delegateClass;

    // this function is used to load and start the loader
    private void loadApplication(Bundle loaderParams) {
        try {
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
            if (!(Boolean) prepareAppMethod.invoke(qtLoader, m_context, classLoader, loaderParams)) {
                throw new Exception("");
            }

            QtApplication.setQtContextDelegate(m_delegateClass, qtLoader);

            // now load the application library so it's accessible from this class loader
            if (libName != null) {
                System.loadLibrary(libName);
            }

            Method startAppMethod = qtLoader.getClass().getMethod("startApplication");
            if (!(Boolean) startAppMethod.invoke(qtLoader)) {
                throw new Exception("");
            }
        } catch (Exception e) {
            e.printStackTrace();
            AlertDialog errorDialog = new AlertDialog.Builder(m_context).create();
            errorDialog.setMessage("Fatal error, your application can't be started.");
            errorDialog.setButton(DialogInterface.BUTTON_NEUTRAL, m_context.getResources().getString(android.R.string.ok), (dialog, which) -> {
                finish();
            });
            errorDialog.show();
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

    public void startApp() {
        try {
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
            if (m_contextInfo.metaData.containsKey("android.app.static_init_classes")) {
                loaderParams.putStringArray(STATIC_INIT_CLASSES_KEY,
                    m_contextInfo.metaData.getString("android.app.static_init_classes").split(":"));
            }
            loaderParams.putStringArrayList(NATIVE_LIBRARIES_KEY, libraryList);
            String themePath = m_context.getApplicationInfo().dataDir + "/qt-reserved-files/android-style/";
            String stylePath = themePath + m_displayDensity + "/";
            String extractOption = "full";
            if (m_contextInfo.metaData.containsKey("android.app.extract_android_style")) {
                extractOption = m_contextInfo.metaData.getString("android.app.extract_android_style");
                if (!extractOption.equals("full") && !extractOption.equals("minimal") && !extractOption.equals("none")) {
                    Log.e(QtApplication.QtTAG, "Invalid extract_android_style option \"" + extractOption + "\", defaulting to full");
                    extractOption = "full";
                }
            }

            if (!(new File(stylePath)).exists() && !extractOption.equals("none")) {
                loaderParams.putString(EXTRACT_STYLE_KEY, stylePath);
                loaderParams.putBoolean(EXTRACT_STYLE_MINIMAL_KEY, extractOption.equals("minimal"));
            }

            if (extractOption.equals("full")) {
                ENVIRONMENT_VARIABLES += "\tQT_USE_ANDROID_NATIVE_STYLE=1";
            }

            ENVIRONMENT_VARIABLES += "\tQT_ANDROID_THEMES_ROOT_PATH=" + themePath;

            loaderParams.putString(ENVIRONMENT_VARIABLES_KEY, ENVIRONMENT_VARIABLES
                + "\tQML2_IMPORT_PATH=" + pluginsPrefix + "/qml"
                + "\tQML_IMPORT_PATH=" + pluginsPrefix + "/imports"
                + "\tQT_PLUGIN_PATH=" + pluginsPrefix + "/plugins");

            String appParams = null;
            if (APPLICATION_PARAMETERS != null) {
                appParams = APPLICATION_PARAMETERS;
            }

            Intent intent = getIntent();
            if (intent != null) {
                String parameters = intent.getStringExtra("applicationArguments");
                if (parameters != null) {
                    if (appParams == null) {
                        appParams = parameters;
                    } else {
                        appParams += '\t' + parameters;
                    }
                }
            }

            if (m_contextInfo.metaData.containsKey("android.app.arguments")) {
                String parameters = m_contextInfo.metaData.getString("android.app.arguments");
                if (appParams == null) {
                    appParams = parameters;
                } else {
                    appParams += '\t' + parameters;
                }
            }

            if (appParams != null) {
                loaderParams.putString(APPLICATION_PARAMETERS_KEY, appParams.replace(' ', '\t').trim());
            }
            loadApplication(loaderParams);
        } catch (Exception e) {
            Log.e(QtApplication.QtTAG, "Can't create main activity", e);
        }
    }

    QtActivity m_activity;

    QtActivityLoader(QtActivity activity) {
        m_context = activity;
        m_delegateClass = QtActivity.class;
        m_activity = activity;
    }

    protected String loaderClassName() {
        return "org.qtproject.qt5.android.QtActivityDelegate";
    }

    protected Class<?> contextClassName() {
        return android.app.Activity.class;
    }

    protected void finish() {
        m_activity.finish();
    }

    protected String getTitle() {
        return (String) m_activity.getTitle();
    }

    protected void runOnUiThread(Runnable run) {
        m_activity.runOnUiThread(run);
    }

    Intent getIntent() {
        return m_activity.getIntent();
    }

    public void onCreate(Bundle savedInstanceState) {
        try {
            m_contextInfo = m_activity.getPackageManager().getActivityInfo(m_activity.getComponentName(), PackageManager.GET_META_DATA);
            int theme = ((ActivityInfo) m_contextInfo).getThemeResource();
            for (Field f : Class.forName("android.R$style").getDeclaredFields()) {
                if (f.getInt(null) == theme) {
                    QT_ANDROID_THEMES = new String[]{f.getName()};
                    QT_ANDROID_DEFAULT_THEME = f.getName();
                    break;
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
            finish();
            return;
        }

        try {
            m_activity.setTheme(Class.forName("android.R$style").getDeclaredField(QT_ANDROID_DEFAULT_THEME).getInt(null));
        } catch (Exception e) {
            e.printStackTrace();
        }

        m_activity.requestWindowFeature(Window.FEATURE_ACTION_BAR);

        if (QtApplication.m_delegateObject != null && QtApplication.onCreate != null) {
            QtApplication.invokeDelegateMethod(QtApplication.onCreate, savedInstanceState);
            return;
        }

        m_displayDensity = m_activity.getResources().getDisplayMetrics().densityDpi;

        ENVIRONMENT_VARIABLES += "\tQT_ANDROID_THEME=" + QT_ANDROID_DEFAULT_THEME
            + "/\tQT_ANDROID_THEME_DISPLAY_DPI=" + m_displayDensity + "\t";

        if (null == m_activity.getLastNonConfigurationInstance()) {
            if (m_contextInfo.metaData.containsKey("android.app.background_running")
                && m_contextInfo.metaData.getBoolean("android.app.background_running")) {
                ENVIRONMENT_VARIABLES += "QT_BLOCK_EVENT_LOOPS_WHEN_SUSPENDED=0\t";
            } else {
                ENVIRONMENT_VARIABLES += "QT_BLOCK_EVENT_LOOPS_WHEN_SUSPENDED=1\t";
            }

            if (m_contextInfo.metaData.containsKey("android.app.auto_screen_scale_factor")
                && m_contextInfo.metaData.getBoolean("android.app.auto_screen_scale_factor")) {
                ENVIRONMENT_VARIABLES += "QT_AUTO_SCREEN_SCALE_FACTOR=1\t";
            }

            startApp();
        }
    }
}
