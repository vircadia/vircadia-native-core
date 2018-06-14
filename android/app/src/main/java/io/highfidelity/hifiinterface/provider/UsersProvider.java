package io.highfidelity.hifiinterface.provider;

import java.util.List;

import io.highfidelity.hifiinterface.view.UserListAdapter;

/**
 * Created by cduarte on 6/13/18.
 */

public interface UsersProvider {

    void retrieve(UsersProvider.UsersCallback usersCallback);

    interface UsersCallback {
        void retrieveOk(List<UserListAdapter.User> users);
        void retrieveError(Exception e, String message);
    }

}
