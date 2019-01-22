package io.highfidelity.hifiinterface.provider;

import android.content.Context;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

import io.highfidelity.hifiinterface.view.AvatarAdapter;

/**
 * Created by gcalero on 1/21/19
 */
public class AvatarProvider {

    private static final String AVATARS_JSON = "avatars.json";
    private static final String JSON_FIELD_NAME = "name";
    private static final String JSON_FIELD_URL = "url";
    private static final String JSON_FIELD_IMAGE = "preview_image";
    private static final String JSON_FIELD_AVATARS_ARRAY = "avatars";
    private final Context mContext;

    public interface AvatarsCallback {
        void retrieveOk(List<AvatarAdapter.Avatar> avatars);
        void retrieveError(Exception e, String message);
    }

    public AvatarProvider(Context context) {
        mContext = context;
    }

    public void retrieve(AvatarsCallback avatarsCallback)
    {
        try {
            JSONObject obj = new JSONObject(loadJSONFromAssets());
            JSONArray m_jArry = obj.getJSONArray(JSON_FIELD_AVATARS_ARRAY);
            ArrayList<AvatarAdapter.Avatar> avatars = new ArrayList<>();

            for (int i = 0; i < m_jArry.length(); i++) {
                JSONObject jo_inside = m_jArry.getJSONObject(i);
                AvatarAdapter.Avatar anAvatar = new AvatarAdapter.Avatar();
                anAvatar.avatarName = jo_inside.getString(JSON_FIELD_NAME);
                anAvatar.avatarPreviewUrl = jo_inside.getString(JSON_FIELD_IMAGE);
                anAvatar.avatarUrl = jo_inside.getString(JSON_FIELD_URL);
                avatars.add(anAvatar);
            }
            avatarsCallback.retrieveOk(avatars);
        } catch (IOException e) {
            avatarsCallback.retrieveError(e, "Failed retrieving avatar JSON");
        } catch (JSONException e) {
            avatarsCallback.retrieveError(e, "Failed parsing avatar JSON");
        }
    }

    private String loadJSONFromAssets() throws IOException {
        String json = null;
        InputStream is = mContext.getAssets().open(AVATARS_JSON);
        int size = is.available();
        byte[] buffer = new byte[size];
        is.read(buffer);
        is.close();
        json = new String(buffer, "UTF-8");
        return json;
    }
}
