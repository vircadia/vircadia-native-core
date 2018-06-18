package io.highfidelity.hifiinterface;

import android.app.Service;
import android.content.Intent;
import android.os.FileObserver;
import android.os.IBinder;
import android.support.annotation.Nullable;
import android.util.Log;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLEncoder;
import java.util.Timer;
import java.util.TimerTask;

import javax.net.ssl.HttpsURLConnection;

public class BreakpadUploaderService extends Service {

    private static final String ANNOTATIONS_JSON = "annotations.json";
    private static final String TAG = "Interface";
    public static final String EXT_DMP = "dmp";
    private static final long DUMP_DELAY = 5000;

    private FileObserver fileObserver;

    public BreakpadUploaderService() {
        super();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        URL baseUrl;

        baseUrl = getUrl();

        if (baseUrl == null) {
            stopSelf();
            return START_NOT_STICKY;
        }

        new Thread(() -> {
            File[] matchingFiles = getFilesByExtension(getObbDir(), EXT_DMP);
            for (File file : matchingFiles) {
                uploadDumpAndDelete(file, baseUrl);
            }
        }).start();

        fileObserver = new FileObserver(getObbDir().getPath()) {
            @Override
            public void onEvent(int event, String path) {
                if (path == null) {
                    return;
                }
                if (FileObserver.CREATE == event && EXT_DMP.equals(getExtension(path))) {
                    URL baseUrl = getUrl();
                    if (baseUrl != null) {
                        new Timer().schedule(new TimerTask() {
                            @Override
                            public void run() {
                                uploadDumpAndDelete(new File(getObbDir(), path), baseUrl);
                            }
                        }, DUMP_DELAY);
                    }
                }
            }
        };

        fileObserver.startWatching();
        return START_STICKY;
    }

    private URL getUrl() {
        String parameters = getAnnotationsAsUrlEncodedParameters();
        try {
            return new URL(BuildConfig.BACKTRACE_URL+ "/post?format=minidump&token=" + BuildConfig.BACKTRACE_TOKEN + (parameters.isEmpty() ? "" : ("&" + parameters)));
        } catch (MalformedURLException e) {
            Log.e(TAG, "Could not initialize Breakpad URL", e);
        }
        return null;
    }

    private void uploadDumpAndDelete(File file, URL url) {
        int size = (int) file.length();
        byte[] bytes = new byte[size];
        try {
            BufferedInputStream buf = new BufferedInputStream(new FileInputStream(file));
                buf.read(bytes, 0, bytes.length);
            buf.close();
            HttpsURLConnection urlConnection = (HttpsURLConnection) url.openConnection();
            urlConnection.setRequestMethod("POST");
            urlConnection.setDoOutput(true);
            urlConnection.setChunkedStreamingMode(0);

            OutputStream ostream = urlConnection.getOutputStream();

            OutputStream out = new BufferedOutputStream(ostream);
            out.write(bytes, 0, size);

            InputStream in = new BufferedInputStream(urlConnection.getInputStream());
            in.read();
            if (urlConnection.getResponseCode() == 200) {
                file.delete();
            }
            urlConnection.disconnect();
        } catch (IOException e) {
            Log.e(TAG, "Error uploading file " + file.getAbsolutePath(), e);
        }
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private File[] getFilesByExtension(File dir, final String extension) {
        return dir.listFiles(pathName -> getExtension(pathName.getName()).equals(extension));
    }

    private String getExtension(String fileName) {
        String extension = "";

        int i = fileName.lastIndexOf('.');
        int p = Math.max(fileName.lastIndexOf('/'), fileName.lastIndexOf('\\'));

        if (i > p) {
            extension = fileName.substring(i+1);
        }

        return extension;
    }


    public String getAnnotationsAsUrlEncodedParameters() {
        String parameters = "";
        File annotationsFile = new File(getObbDir(), ANNOTATIONS_JSON);
        if (annotationsFile.exists()) {
            JsonParser parser = new JsonParser();
            try {
                JsonObject json = (JsonObject) parser.parse(new FileReader(annotationsFile));
                for (String k: json.keySet()) {
                    if (!json.get(k).getAsString().isEmpty()) {
                        String key = k.contains("/") ? k.substring(k.indexOf("/") + 1) : k;
                        if (!parameters.isEmpty()) {
                            parameters += "&";
                        }
                        parameters += URLEncoder.encode(key, "UTF-8") + "=" + URLEncoder.encode(json.get(k).getAsString(), "UTF-8");
                    }
                }
            } catch (FileNotFoundException e) {
                Log.e(TAG, "Error reading annotations file", e);
            } catch (UnsupportedEncodingException e) {
                Log.e(TAG, "Error reading annotations file", e);
            }
        }
        return parameters;
    }

}
