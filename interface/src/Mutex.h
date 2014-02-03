/** 
 *	@file Mutex.h
 *	An operating system independent simple wrapper to mutex services.
 *
 *	@author: Norman Crafts
 *	@copyright 2014, All rights reserved.
 */
#ifndef __MUTEX_H__
#define __MUTEX_H__

#ifdef LINUX
#include <errno.h>
#include <pthread.h>
#endif

/**
 *	@class Mutex
 *
 *	This class is an OS independent delegate pattern providing access
 *	to simple OS dependent mutex services. Assume the mutex service is
 *	non-reentrant.
 */
class Mutex
{
public:
	/** Default constructor
	 */
	Mutex();

	~Mutex();

	/** Lock the mutex barrier
	 *	First competing thread will capture the mutex and block all
	 *	other threads.
	 */
	void lock();

	/** Unlock the mutex barrier
	 *	Mutex owner releases the mutex and allow competing threads to try to capture.
	 */
	void unlock();

private:
	/** Copy constructor prohibited to API user
	 */
	Mutex(
		Mutex const & copy
		);

private:

#ifdef _DEBUG
	int _lockCt;
#endif	//_DEBUG

#ifdef WIN32
	CRITICAL_SECTION _mutex;
#endif	// WIN32

#ifdef LINUX
	pthread_mutex_t _mutex;
#endif	// LINUX
};


#ifdef WIN32

inline
Mutex::Mutex()
#ifdef _DEBUG
	:
		_lockCt(0)
#endif
{
	InitializeCriticalSection(&_mutex);
}

inline
Mutex::~Mutex()
{
	DeleteCriticalSection(&_mutex);
}

inline
void Mutex::lock()
{
	EnterCriticalSection(&_mutex);
	#ifdef _DEBUG
		_lockCt++;
	#endif	// _DEBUG
}

inline
void Mutex::unlock()
{
	#ifdef _DEBUG
		_lockCt--;
	#endif	// _DEBUG
	LeaveCriticalSection(&_mutex);
}

#endif	// WIN32

#ifdef LINUX

inline
Mutex::Mutex()
#ifdef _DEBUG
	:
		_lockCt(0)
#endif
{
	int err;
	pthread_mutexattr_t attrib;

	err = pthread_mutexattr_init(&attrib);
	err = pthread_mutex_init(&_mutex, &attrib);
	err = pthread_mutexattr_destroy(&attrib);
}

inline
Mutex::~Mutex()
{
	pthread_mutex_destroy(&_mutex);
}

inline
void Mutex::lock()
{
	pthread_mutex_lock(&_mutex);
	#ifdef _DEBUG
		_lockCt++;
	#endif	// _DEBUG
}

inline
void Mutex::unlock()
{
	#ifdef _DEBUG
		_lockCt--;
	#endif	// _DEBUG
	pthread_mutex_unlock(&_mutex);
}

#endif	// LINUX
#endif	// __MUTEX_H__
