package io.highfidelity.hifiinterface.provider;

import java.util.List;

import io.highfidelity.hifiinterface.view.DomainAdapter;

/**
 * Created by cduarte on 4/17/18.
 */

public interface DomainProvider {

    void retrieve(Callback callback);

    interface Callback {
        void retrieveOk(List<DomainAdapter.Domain> domain);
        void retrieveError(Exception e, String message);
    }
}
