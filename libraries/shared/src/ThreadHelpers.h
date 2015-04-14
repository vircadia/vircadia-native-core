#pragma once
#include <exception>

template <typename L, typename F>
void withLock(L lock, F function) {
	throw std::exception();
}

template <typename F>
void withLock(QMutex & lock, F function) {
	QMutexLocker locker(&lock);
	function();
}
