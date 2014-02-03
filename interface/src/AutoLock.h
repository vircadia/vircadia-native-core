/**
 *	@file AutoLock.h
 *	A simple locking class featuring templated mutex policy and mutex barrier
 *	activation/deactivation constructor/destructor.
 *
 *	@author Norman Crafts
 *	@copyright 2014, All rights reserved.
 */
#ifndef __AUTOLOCK_H__
#define __AUTOLOCK_H__

#include "Mutex.h"

/**
 *	@class AutoLock
 */
template
<
	class MutexPolicy = Mutex
>
class AutoLock
{
public:
	/** Dependency injection constructor
	 *	AutoLock constructor with client mutex injection
	 */
	AutoLock(
		MutexPolicy & client					///< Client mutex
		);

	/** Dependency injection constructor
	 *	AutoLock constructor with client mutex injection
	 */
	AutoLock(
		MutexPolicy *client						///< Client mutex
		);

	~AutoLock();

private:
	/** Default constructor prohibited to API user
	 */
	AutoLock();

	/** Copy constructor prohibited to API user
	 */
	AutoLock(
		AutoLock const & copy
		);

private:
	MutexPolicy &_mutex;						///< Client mutex
};


template<class MutexPolicy>
inline
AutoLock<MutexPolicy>::AutoLock(
	MutexPolicy &mutex
	) :
		_mutex(mutex)
{
	_mutex.lock();
}

template<class MutexPolicy>
inline
AutoLock<MutexPolicy>::AutoLock(
	MutexPolicy *mutex
	) :
		_mutex(*mutex)
{
	_mutex.lock();
}

template<class MutexPolicy>
inline
AutoLock<MutexPolicy>::~AutoLock()
{
	_mutex.unlock();
}


#endif	// __AUTOLOCK_H__
