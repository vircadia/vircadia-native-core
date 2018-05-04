package io.highfidelity.hifiinterface.provider;

import android.util.Log;
import android.util.MutableInt;

import java.util.ArrayList;
import java.util.List;

import io.highfidelity.hifiinterface.HifiUtils;
import io.highfidelity.hifiinterface.view.DomainAdapter;
import retrofit2.Call;
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

    private static final String INCLUDE_ACTIONS_FOR_PLACES = "concurrency";
    private static final String INCLUDE_ACTIONS_FOR_FULL_SEARCH = "concurrency,announcements,snapshot";
    private static final int MAX_PAGES_TO_GET = 10;

    private String mProtocol;
    private Retrofit mRetrofit;
    private UserStoryDomainProviderService mUserStoryDomainProviderService;

    private boolean startedToGetFromAPI = false;
    private List<UserStory> allStories; // All retrieved stories from the API
    private List<DomainAdapter.Domain> suggestions; // Filtered places to show

    public UserStoryDomainProvider(String protocol) {
        mRetrofit = new Retrofit.Builder()
                .baseUrl(BASE_URL)
                .addConverterFactory(GsonConverterFactory.create())
                .build();
        mUserStoryDomainProviderService = mRetrofit.create(UserStoryDomainProviderService.class);
        mProtocol = protocol;
        allStories = new ArrayList<>();
        suggestions = new ArrayList<>();
    }

    private void fillDestinations(String filterText, DomainCallback domainCallback) {
        StoriesFilter filter = new StoriesFilter(filterText);
        final MutableInt counter = new MutableInt(0);
        allStories.clear();
        getUserStoryPage(1,
                e -> {
                    allStories.subList(counter.value, allStories.size()).forEach(userStory -> {
                        filter.filterOrAdd(userStory);
                    });
                    if (domainCallback != null) {
                        domainCallback.retrieveOk(suggestions); //ended
                    }
                },
                a -> {
                    allStories.forEach(userStory -> {
                        counter.value++;
                        filter.filterOrAdd(userStory);
                    });
                }
        );
    }

    private void handleError(String url, Throwable t, Callback<Exception> restOfPagesCallback) {
        restOfPagesCallback.callback(new Exception("Error accessing url [" + url + "]", t));
    }

    private void getUserStoryPage(int pageNumber, Callback<Exception> restOfPagesCallback, Callback<Void> firstPageCallback) {
        Call<UserStories> userStories = mUserStoryDomainProviderService.getUserStories(
                INCLUDE_ACTIONS_FOR_PLACES,
                "open",
                true,
                mProtocol,
                pageNumber);
        Log.d("API-USER-STORY-DOMAINS", "Protocol [" + mProtocol + "] include_actions [" + INCLUDE_ACTIONS_FOR_PLACES + "]");
        userStories.enqueue(new retrofit2.Callback<UserStories>() {
            @Override
            public void onResponse(Call<UserStories> call, Response<UserStories> response) {
                UserStories data = response.body();
                allStories.addAll(data.user_stories);
                if (data.current_page < data.total_pages && data.current_page <= MAX_PAGES_TO_GET) {
                    if (pageNumber == 1 && firstPageCallback != null) {
                        firstPageCallback.callback(null);
                    }
                    getUserStoryPage(pageNumber + 1, restOfPagesCallback, null);
                    return;
                }
                restOfPagesCallback.callback(null);
            }

            @Override
            public void onFailure(Call<UserStories> call, Throwable t) {
                handleError(call.request().url().toString(), t, restOfPagesCallback);
            }
        });
    }

    private class StoriesFilter {
        String[] mWords = new String[]{};
        public StoriesFilter(String filterText) {
            mWords = filterText.toUpperCase().split("\\s+");
        }

        private boolean matches(UserStory story) {
            if (mWords.length <= 0) {
                return true;
            }

            for (String word : mWords) {
                if (!story.searchText().contains(word)) {
                    return false;
                }
            }

            return true;
        }

        private void addToSuggestions(UserStory story) {
            suggestions.add(story.toDomain());
        }

        public void filterOrAdd(UserStory story) {
            if (matches(story)) {
                addToSuggestions(story);
            }
        }
    }

    private void filterChoicesByText(String filterText, DomainCallback domainCallback) {
        suggestions.clear();
        StoriesFilter storiesFilter = new StoriesFilter(filterText);
        allStories.forEach(story -> {
            storiesFilter.filterOrAdd(story);
        });
        domainCallback.retrieveOk(suggestions);
    }

    @Override
    public synchronized void retrieve(String filterText, DomainCallback domainCallback) {
        if (!startedToGetFromAPI) {
            startedToGetFromAPI = true;
            fillDestinations(filterText, domainCallback);
        } else {
            filterChoicesByText(filterText, domainCallback);
        }
    }

    public void retrieveNot(DomainCallback domainCallback) {
        // TODO Call multiple pages
        Call<UserStories> userStories = mUserStoryDomainProviderService.getUserStories(
                INCLUDE_ACTIONS_FOR_PLACES,
                "open",
                true,
                mProtocol,
                1);

        Log.d("API-USER-STORY-DOMAINS", "Protocol [" + mProtocol + "] include_actions [" + INCLUDE_ACTIONS_FOR_PLACES + "]");
        userStories.enqueue(new retrofit2.Callback<UserStories>() {

            @Override
            public void onResponse(Call<UserStories> call, Response<UserStories> response) {
                UserStories userStories = response.body();
                if (userStories == null) {
                    domainCallback.retrieveOk(new ArrayList<>(0));
                }
                List<DomainAdapter.Domain> domains = new ArrayList<>(userStories.total_entries);
                userStories.user_stories.forEach(userStory -> {
                    domains.add(userStory.toDomain());
                });
                domainCallback.retrieveOk(domains);
            }

            @Override
            public void onFailure(Call<UserStories> call, Throwable t) {
                domainCallback.retrieveError(new Exception(t), t.getMessage());
            }

        });
    }

    public interface UserStoryDomainProviderService {
        @GET("api/v1/user_stories")
        Call<UserStories> getUserStories(@Query("include_actions") String includeActions,
                                         @Query("restriction") String restriction,
                                         @Query("require_online") boolean requireOnline,
                                         @Query("protocol") String protocol,
                                         @Query("page") int pageNumber);
    }

    class UserStory {
        public UserStory() {}
        String place_name;
        String path;
        String thumbnail_url;
        String place_id;
        String domain_id;
        private String searchText;

        // New fields? tags, description

        String searchText() {
            if (searchText == null) {
                searchText = place_name == null? "" : place_name.toUpperCase();
            }
            return searchText;
        }
        DomainAdapter.Domain toDomain() {
            // TODO Proper url creation (it can or can't have hifi
            // TODO Or use host value from api?
            String absoluteThumbnailUrl = HifiUtils.getInstance().absoluteHifiAssetUrl(thumbnail_url);
            DomainAdapter.Domain domain = new DomainAdapter.Domain(
                    place_name,
                    HifiUtils.getInstance().sanitizeHifiUrl(place_name) + "/" + path,
                    absoluteThumbnailUrl
            );
            return domain;
        }
    }

    class UserStories {
        String status;
        int current_page;
        int total_pages;
        int total_entries;
        List<UserStory> user_stories;
    }

}
