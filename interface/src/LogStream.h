//
// LogStream.h
// interface
//
// Created by Tobias Schwinger on 4/17/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__LogStream__
#define __interface__LogStream__

#include <sstream>

#include "Log.h"

//
// Makes the logging facility accessible as a C++ stream.
//
// Example: 
//
//      // somewhere central - ideally one per thread (else pass 'true' as
//      // second constructor argument and compromise some efficiency)
//      LogStream lOut(printLog); 
//
//      // elsewhere:
//      lOut << "Hello there!" << std::endl;
//
class LogStream {
    std::ostringstream  _outStream;
    Log&                _logRef;
    bool                _isThreadSafe;
public:
    inline LogStream(Log& log, bool threadSafe = false);

    class StreamRef; friend class StreamRef;

    template< typename T > friend inline LogStream::StreamRef const operator<<(LogStream&, T const&);

private:
    // don't
    LogStream(LogStream const&); // = delete;
    LogStream& operator=(LogStream const&); // = delete;

    inline void ostreamBegin();
    inline void ostreamEnd();
};

inline LogStream::LogStream(Log& log, bool threadSafe) :
    _outStream(std::ios_base::out), _logRef(log), _isThreadSafe(threadSafe) { }

inline void LogStream::ostreamBegin() {

    if (_isThreadSafe) {
        // the user wants to share this LogStream among threads,
        // so lock the global log here, already
        pthread_mutex_lock(& _logRef._mutex);
    }
    _outStream.str("");
}

inline void LogStream::ostreamEnd() {

    if (! _isThreadSafe) {
        // haven't locked, so far (we have memory for each thread)
        pthread_mutex_lock(& _logRef._mutex);
    }
    _logRef.addMessage(_outStream.str().c_str());
    pthread_mutex_unlock(& _logRef._mutex);
}


//
// The Log::StreamRef class makes operator<< work. It...
//
class LogStream::StreamRef {
    mutable LogStream* _logStream;
    typedef std::ostream& (*manipulator)(std::ostream&);

    friend class LogStream;

    template< typename T > friend inline LogStream::StreamRef const operator<<(LogStream&, T const&);
    StreamRef(LogStream* log) : _logStream(log) { }
public:
    // ...forwards << operator calls to stringstream...
    template< typename T > StreamRef const operator<<(T const& x) const { _logStream->_outStream << x; return *this; }
    // ...has to dance around to make manipulators (such as std::hex, std::endl) work...
    StreamRef const operator<<(manipulator x) const { _logStream->_outStream << x; return *this; }
    // ...informs the logger that a stream has ended when it has the responsibility...
    ~StreamRef() { if (_logStream != 0l) { _logStream->ostreamEnd(); } }
    // ...which is passed on upon copy. 
    StreamRef(StreamRef const& other) : _logStream(other._logStream) { other._logStream = 0l; }

private:
    // don't
    StreamRef& operator=(StreamRef const&); // = delete; 
};

template< typename T > inline LogStream::StreamRef const operator<<(LogStream& s, T const& x) {
    
    s.ostreamBegin(); 
    s._outStream << x;
    return LogStream::StreamRef(& s); // calls streamEnd at the end of the stream expression
}


#endif
