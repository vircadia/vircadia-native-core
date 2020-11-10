package io.highfidelity.hifiinterface.provider;

import android.util.Log;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

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

    public static final String BASE_URL = "https://metaverse.vircadia.com/live/";

    private static final String INCLUDE_ACTIONS_FOR_PLACES = "concurrency";
    private static final String INCLUDE_ACTIONS_FOR_FULL_SEARCH = "concurrency,announcements,snapshot";
    private static final String TAGS_TO_SEARCH = "mobile";
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

    @Override
    public synchronized void retrieve(String filterText, DomainCallback domainCallback, boolean forceRefresh) {
        if (!startedToGetFromAPI || forceRefresh) {
            startedToGetFromAPI = true;
            fillDestinations(filterText, domainCallback);
        } else {
            filterChoicesByText(filterText, domainCallback);
        }
    }

    private void fillDestinations(String filterText, DomainCallback domainCallback) {
        StoriesFilter filter = new StoriesFilter(filterText);

        List<UserStory> taggedStories = new ArrayList<>();
        Set<String> taggedStoriesIds = new HashSet<>();
        getUserStoryPage(1, taggedStories, TAGS_TO_SEARCH,
                e -> {
                    taggedStories.forEach(userStory -> {
                        taggedStoriesIds.add(userStory.id);
                    });

                    allStories.clear();
                    getUserStoryPage(1, allStories, null,
                            ex -> {
                                suggestions.clear();
                                allStories.forEach(userStory -> {
                                    if (taggedStoriesIds.contains(userStory.id)) {
                                        userStory.tagFound = true;
                                    }
                                    filter.filterOrAdd(userStory);
                                });
                                if (domainCallback != null) {
                                    domainCallback.retrieveOk(suggestions); //ended
                                }
                            }
                    );

                }
        );
    }

    private void handleError(String url, Throwable t, Callback<Exception> restOfPagesCallback) {
        restOfPagesCallback.callback(new Exception("Error accessing url [" + url + "]", t));
    }

    private void getUserStoryPage(int pageNumber, List<UserStory> userStoriesList, String tagsFilter, Callback<Exception> restOfPagesCallback) {
        Call<UserStories> userStories = mUserStoryDomainProviderService.getUserStories(
                INCLUDE_ACTIONS_FOR_PLACES,
                "open",
                true,
                mProtocol,
                tagsFilter,
                pageNumber);
        Log.d("API-USER-STORY-DOMAINS", "Protocol [" + mProtocol + "] include_actions [" + INCLUDE_ACTIONS_FOR_PLACES + "]");
        userStories.enqueue(new retrofit2.Callback<UserStories>() {
            @Override
            public void onResponse(Call<UserStories> call, Response<UserStories> response) {
                UserStories data = response.body();
                userStoriesList.addAll(data.user_stories);
                if (data.current_page < data.total_pages && data.current_page <= MAX_PAGES_TO_GET) {
                    getUserStoryPage(pageNumber + 1, userStoriesList, tagsFilter, restOfPagesCallback);
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
            mWords = filterText.trim().toUpperCase().split("\\s+");
            if (mWords.length == 1 && (mWords[0] == null || mWords[0].length() <= 0 ) ) {
                mWords = null;
            }
        }

        private boolean matches(UserStory story) {
            if (mWords == null || mWords.length <= 0) {
                // No text filter? So filter by tag
                return story.tagFound;
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

        /**
         * if the story matches this filter criteria it's added into suggestions
         * */
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

    public interface UserStoryDomainProviderService {
        @GET("/api/v1/user_stories")
        Call<UserStories> getUserStories(@Query("include_actions") String includeActions,
                                         @Query("restriction") String restriction,
                                         @Query("require_online") boolean requireOnline,
                                         @Query("protocol") String protocol,
                                         @Query("tags") String tagsCommaSeparated,
                                         @Query("page") int pageNumber);
    }

    class UserStory {
        public UserStory() {}
        String id;
        String place_name;
        String path;
        String thumbnail_url;
        String place_id;
        String domain_id;
        private String searchText;
        private boolean tagFound; // Locally used

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
