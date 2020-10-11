package io.highfidelity.hifiinterface.provider;

import android.util.Log;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import io.highfidelity.hifiinterface.view.UserListAdapter;
import okhttp3.Interceptor;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Retrofit;
import retrofit2.converter.gson.GsonConverterFactory;
import retrofit2.http.Body;
import retrofit2.http.DELETE;
import retrofit2.http.GET;
import retrofit2.http.POST;
import retrofit2.http.Path;
import retrofit2.http.Query;

/**
 * Created by cduarte on 6/13/18.
 */

public class EndpointUsersProvider implements UsersProvider {

    public static final String BASE_URL = "https://metaverse.vircadia.com/live/";
    private final Retrofit mRetrofit;
    private final EndpointUsersProviderService mEndpointUsersProviderService;

    public EndpointUsersProvider(String accessToken) {
        mRetrofit = createAuthorizedRetrofit(accessToken);
        mEndpointUsersProviderService = mRetrofit.create(EndpointUsersProviderService.class);
    }

    private Retrofit createAuthorizedRetrofit(String accessToken) {
        Retrofit mRetrofit;
        OkHttpClient.Builder httpClient = new OkHttpClient.Builder();
        httpClient.addInterceptor(new Interceptor() {
            @Override
            public Response intercept(Chain chain) throws IOException {
                Request original = chain.request();

                Request request = original.newBuilder()
                        .header("Authorization", "Bearer " + accessToken)
                        .method(original.method(), original.body())
                        .build();

                return chain.proceed(request);
            }
        });

        OkHttpClient client = httpClient.build();

        mRetrofit = new Retrofit.Builder()
            .baseUrl(BASE_URL)
            .addConverterFactory(GsonConverterFactory.create())
            .client(client)
            .build();
        return mRetrofit;
    }

    @Override
    public void retrieve(UsersCallback usersCallback) {
        Call<UsersResponse> friendsCall = mEndpointUsersProviderService.getUsers(
                CONNECTION_FILTER_CONNECTIONS,
                400,
                null);
        friendsCall.enqueue(new Callback<UsersResponse>() {
            @Override
            public void onResponse(Call<UsersResponse> call, retrofit2.Response<UsersResponse> response) {
                if (!response.isSuccessful()) {
                    usersCallback.retrieveError(new Exception("Error calling Users API"), "Error calling Users API");
                    return;
                }
                UsersResponse usersResponse = response.body();
                List<UserListAdapter.User> adapterUsers = new ArrayList<>(usersResponse.total_entries);
                for (User user : usersResponse.data.users) {
                    UserListAdapter.User adapterUser = new UserListAdapter.User();
                    adapterUser.connection = user.connection;
                    adapterUser.imageUrl = user.images.thumbnail;
                    adapterUser.name = user.username;
                    adapterUser.online = user.online;
                    adapterUser.locationName = (user.location != null ?
                            (user.location.root != null ? user.location.root.name :
                                    (user.location.domain != null ? user.location.domain.name : ""))
                            : "");
                    adapterUsers.add(adapterUser);
                }
                usersCallback.retrieveOk(adapterUsers);
            }

            @Override
            public void onFailure(Call<UsersResponse> call, Throwable t) {
                usersCallback.retrieveError(new Exception(t), "Error calling Users API");
            }
        });
    }

    public class UserActionRetrofitCallback implements Callback<UsersResponse> {

        UserActionCallback callback;

        public UserActionRetrofitCallback(UserActionCallback callback) {
            this.callback = callback;
        }

        @Override
        public void onResponse(Call<UsersResponse> call, retrofit2.Response<UsersResponse> response) {
            if (!response.isSuccessful()) {
                callback.requestError(new Exception("Error with "
                                + call.request().url().toString() + " "
                                + call.request().method() + " call " + response.message()),
                        response.message());
                return;
            }

            if (response.body() == null || !"success".equals(response.body().status)) {
                callback.requestError(new Exception("Error with "
                                + call.request().url().toString() + " "
                                + call.request().method() + " call " + response.message()),
                        response.message());
                return;
            }
            callback.requestOk();
        }

        @Override
        public void onFailure(Call<UsersResponse> call, Throwable t) {
            callback.requestError(new Exception(t), t.getMessage());
        }
    }

    @Override
    public void addFriend(String friendUserName, UserActionCallback callback) {
        Call<UsersResponse> friendCall = mEndpointUsersProviderService.addFriend(new BodyAddFriend(friendUserName));
        friendCall.enqueue(new UserActionRetrofitCallback(callback));
    }

    @Override
    public void removeFriend(String friendUserName, UserActionCallback callback) {
        Call<UsersResponse> friendCall = mEndpointUsersProviderService.removeFriend(friendUserName);
        friendCall.enqueue(new UserActionRetrofitCallback(callback));
    }

    @Override
    public void removeConnection(String connectionUserName, UserActionCallback callback) {
        Call<UsersResponse> connectionCall = mEndpointUsersProviderService.removeConnection(connectionUserName);
        connectionCall.enqueue(new UserActionRetrofitCallback(callback));
    }

    public interface EndpointUsersProviderService {
        @GET("/api/v1/users")
        Call<UsersResponse> getUsers(@Query("filter") String filter,
                                           @Query("per_page") int perPage,
                                           @Query("online") Boolean online);

        @DELETE("/api/v1/user/connections/{connectionUserName}")
        Call<UsersResponse> removeConnection(@Path("connectionUserName") String connectionUserName);

        @DELETE("/api/v1/user/friends/{friendUserName}")
        Call<UsersResponse> removeFriend(@Path("friendUserName") String friendUserName);

        @POST("/api/v1/user/friends")
        Call<UsersResponse> addFriend(@Body BodyAddFriend friendUserName);

        /* response
         {
            "status": "success"
        }
        */
    }

    class BodyAddFriend {
        String username;
        public BodyAddFriend(String username) {
            this.username = username;
        }
    }

    class UsersResponse {
        public UsersResponse() {}
        String status;
        int current_page;
        int total_pages;
        int per_page;
        int total_entries;
        Data data;
    }

    class Data {
        public Data() {}
        List<User> users;
    }

    class User {
        public User() {}
        String username;
        boolean online;
        String connection;
        Images images;
        LocationData location;
    }

    class Images {
        public Images() {}
        String hero;
        String thumbnail;
        String tiny;
    }

    class LocationData {
        public LocationData() {}
        NameContainer root;
        NameContainer domain;
    }
    class NameContainer {
        String name;
    }

}
