package io.highfidelity.hifiinterface.provider;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

import io.highfidelity.hifiinterface.view.DomainAdapter;
import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;
import retrofit2.Retrofit;
import retrofit2.converter.gson.GsonConverterFactory;
import retrofit2.http.GET;
import retrofit2.http.Query;

/**
 * Created by cduarte on 4/17/18.
 */

public class UserStoryDomainProvider implements DomainProvider {

    public static final String BASE_URL = "https://metaverse.highfidelity.com/";

    private String mProtocol;
    private Retrofit mRetrofit;
    private UserStoryDomainProviderService mUserStoryDomainProviderService;

    public UserStoryDomainProvider(String protocol) {
        mRetrofit = new Retrofit.Builder()
                .baseUrl(BASE_URL)
                .addConverterFactory(GsonConverterFactory.create())
                .build();
        mUserStoryDomainProviderService = mRetrofit.create(UserStoryDomainProviderService.class);
        mProtocol = protocol;
    }

    @Override
    public void retrieve(Callback callback) {
        // TODO Call multiple pages
        Call<UserStories> userStories = mUserStoryDomainProviderService.getUserStories(mProtocol);
        userStories.enqueue(new retrofit2.Callback<UserStories>() {

            @Override
            public void onResponse(Call<UserStories> call, Response<UserStories> response) {
                UserStories userStories = response.body();
                if (userStories == null) {
                    callback.retrieveOk(new ArrayList<>(0));
                }
                List<DomainAdapter.Domain> domains = new ArrayList<>(userStories.total_entries);
                userStories.user_stories.forEach(userStory -> {
                    // TODO Proper url creation (it can or can't have hifi
                    // TODO Or use host value from api?
                    domains.add(new DomainAdapter.Domain(
                            userStory.place_name,
                            "hifi://" + userStory.place_name + "/" + userStory.path,
                            userStory.thumbnail_url
                    ));
                });
                callback.retrieveOk(domains);
            }

            @Override
            public void onFailure(Call<UserStories> call, Throwable t) {
                callback.retrieveError(new Exception(t), t.getMessage());
            }

        });
    }

    public interface UserStoryDomainProviderService {
        @GET("api/v1/user_stories")
        Call<UserStories> getUserStories(@Query("protocol") String protocol);
    }

    class UserStory {
        public UserStory() {}
        String place_name;
        String path;
        String thumbnail_url;
    }

    class UserStories {
        String status;
        int current_page;
        int total_pages;
        int total_entries;
        List<UserStory> user_stories;
    }
}
