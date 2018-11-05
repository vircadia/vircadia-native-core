package io.highfidelity.hifiinterface.provider;

import java.util.List;

import io.highfidelity.hifiinterface.view.DomainAdapter;

/**
 * Created by cduarte on 4/17/18.
 */

public interface DomainProvider {

    void retrieve(String filterText, DomainCallback domainCallback, boolean forceRefresh);

    interface DomainCallback {
        void retrieveOk(List<DomainAdapter.Domain> domain);
        void retrieveError(Exception e, String message);
    }
}
