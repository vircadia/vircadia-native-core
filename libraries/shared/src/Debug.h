//
//  Debug.h
//  libraries/shared/src
//
//  Created by Zach Pomerantz on 4/12/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Debug-specific classes.
//  To use without drastically increasing compilation time, #include <Debug.h> in your cpp files only.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Debug_h
#define hifi_Debug_h

#ifdef DEBUG

#include <atomic>
#include <QDebug>

// Return a string suffixed with a monotonically increasing ID, as string(ID).
// Useful for Tracker<std::string> instances, when many instances share the same "name".
//
// K must have an implemented streaming operator for QDebug operator<<. (Done for std::string in this header.)
static std::string getUniqString(const std::string& s) {
    static std::atomic<size_t> counter{ 0 };
    return s + '(' + std::to_string(counter.fetch_add(1)) + ')';
}

// Implement the streaming operator for std::string for ease-of-use.
static QDebug operator<<(QDebug debug, const std::string& s) {
    QDebugStateSaver saver(debug);
    debug.nospace() << s.c_str();
    return debug;
}

// A counter. Prints out counts for each key on destruction.
template <class K>
class Counter {
    using Map = std::map<K, size_t>;

public:
    enum LogLevel {
        // Only print counts for each count (e.g. "5 34 entries", meaning 34 keys have a count of 5).
        SUMMARY = 0,
        // Print SUMMARY and every key and its count.
        DETAILED,
    };

    Counter(const std::string& name, LogLevel logLevel = LogLevel::DETAILED) :
        _logLevel{ logLevel } {
        _name = name;
    }
    ~Counter() { log(); }

    // Increase the count for key (by inc).
    void add(const K& key, size_t inc = 1);

    // Decrease the count for key (by dec).
    void sub(const K& key, size_t dec = 1);

    // Log current counts (called on destruction).
    void log();

private:
    std::string _name;
    std::vector<std::string> _legend;
    LogLevel _logLevel;

    std::mutex _mutex;
    Map _map;
};

// A tracker. Tracks keys state (as a size_t, mapped to std::string).
// Prints out counts for each state on destruction. Optionally prints out counts for duplicates and each key's state.
//
// K must have an implemented streaming operator for QDebug operator<<. (Done for std::string in this header.)
//
// Useful cases may be tracking pointers' ctor/dtor (K = size_t or K = uintptr_t), or tracking resource names (K = std::string).
template <class K>
class Tracker {
    using Map = std::map<K, size_t>;
    using DuplicateValueMap = std::map<size_t, size_t>;
    using DuplicateMap = std::map<K, DuplicateValueMap>;

public:
    enum LogLevel {
        // Only print counts for each state.
        SUMMARY = 0,
        // Print SUMMARY and a count of duplicate states (when a key is set to its current state).
        DUPLICATES,
        // Print DUPLICATES and the current state for each key not in its final state (the last state of the legend).
        DETAILED,
    };

    // Create a new tracker.
    // legend:      defines a mapping from state (size_t) to that state's name. The final state is assumed to be the last state in the legend.
    // allowReuse:  if true, keys in the last state may be set to a different state.
    Tracker(const std::string& name, const std::vector<std::string>& legend, LogLevel logLevel = SUMMARY, bool allowReuse = true) :
        _logLevel{ logLevel }, _allowReuse{ allowReuse }, _max{ legend.size() - 1 } {
        _name = name;
        _legend = legend;
    }
    ~Tracker() { log(); }

    // Set key to state.
    // Fails (triggers an assertion) if (state < current state) (i.e. states are ordered),
    // unless allowReuse is true and the current state is the final state.
    void set(const K& key, const size_t& state);

    // Log current states (called on destruction).
    void log();

private:
    std::string _name;
    std::vector<std::string> _legend;
    bool _allowReuse;
    size_t _max;
    LogLevel _logLevel;

    std::mutex _mutex;
    Map _map;
    DuplicateMap _duplicates;
};

template<class K>
void Counter<K>::add(const K& k, size_t inc) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto& it = _map.find(k);

    if (it == _map.end()) {
        // No entry for k; add it
        _map.insert(std::pair<K, size_t>(k, inc));
    } else {
        // Entry for k; update it
        it->second += inc;
    }
}

template<class K>
void Counter<K>::sub(const K& k, size_t dec) {
    add(k, -dec);
}

template <class K>
void Counter<K>::log() {
    // Avoid logging nothing
    if (!_map.size()) return;

    std::map<size_t, size_t> results;
    for (const auto& entry : _map) {
        if (!results.insert(std::pair<size_t, size_t>(entry.second, 1)).second) {
            results.find(entry.second)->second++;
        }
    }

    qCDebug(shared) << "Counts for" << _name;
    for (const auto& entry : results) {
        qCDebug(shared) << entry.first << '\t' << entry.second << "entries";
    }
    if (_logLevel == LogLevel::SUMMARY) return;

    qCDebug(shared) << "Entries";
    for (const auto& entry : _map) {
        qCDebug(shared) << entry.first << '\t' << entry.second;
    }
    if (_logLevel == LogLevel::DETAILED) return;
}

template <class K>
void Tracker<K>::set(const K& k, const size_t& v) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto& it = _map.find(k);

    if (it == _map.end()) {
        // No entry for k; add it
        _map.insert(std::pair<K, size_t>(k, v));
    } else if (v > it->second) {
        // Ordered entry for k; update it
        it->second = v;
    } else if (v == it->second) {
        // Duplicate entry for k; log it
        auto& dit = _duplicates.find(k);

        if (dit == _duplicates.end()) {
            // No duplicate value map for k; add it
            DuplicateValueMap duplicateValueMap{ std::pair<size_t, size_t>(v, 1) };
            _duplicates.insert(std::pair<K, DuplicateValueMap>(k, duplicateValueMap));
        } else {
            // Duplicate value map for k; update it
            auto& dvit = dit->second.find(v);

            if (dvit == dit->second.end()) {
                // No duplicate entry for (k, v); add it
                dit->second.insert(std::pair<size_t, size_t>(v, 1));
            } else {
                // Duplicate entry for (k, v); update it
                dvit->second++;
            }
        }
    } else { // if (v < it.second)
        if (_allowReuse && it->second == _max) {
            // Reusing an entry; update it
            it->second = v;
        } else {
            // Unordered entry for k; dump log and fail
            qCDebug(shared) << "Badly ordered entry detected:" <<
                k << _legend.at(it->second).c_str() << "->" << _legend.at(v).c_str();
            log();
            assert(false);
        }
    }
}

template <class K>
void Tracker<K>::log() {
    // Avoid logging nothing
    if (!_map.size()) return;

    std::vector<std::vector<K>> results(_max + 1);
    for (const auto& entry : _map) {
        results.at(entry.second).push_back(entry.first);
    }

    qCDebug(shared) << "Summary of" << _name;
    for (auto i = 0; i < results.size(); ++i) {
        qCDebug(shared) << _legend.at(i) << '\t' << results[i].size() << "entries";
    }
    if (_logLevel == LogLevel::SUMMARY) return;

    size_t size = 0;
    for (const auto& entry : _duplicates) {
        size += entry.second.size();
    }
    qCDebug(shared) << "Duplicates" << size << "entries";
    // TODO: Add more detail to duplicate logging
    if (_logLevel <= LogLevel::DUPLICATES) return;

    qCDebug(shared) << "Entries";
    // Don't log the terminal case
    for (auto i = 0; i < _max; ++i) {
        qCDebug(shared) << "----" << _legend.at(i) << "----";
        for (const auto& entry : results[i]) {
            qCDebug(shared) << "\t" << entry;
        }
    }
    if (_logLevel <= LogLevel::DETAILED) return;
}

#endif // DEBUG

#endif // hifi_Debug_h

