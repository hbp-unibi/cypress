/*
 *  Cypress -- C++ Neural Associative Memory Simulator
 *  Copyright (C) 2016  Andreas Stöckel
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This code is adapted from
 * http://www.boost.org/doc/libs/1_48_0/doc/html/move/implementing_movable_classes.html
 */

#pragma once

#ifndef CYPRESS_UTIL_CLONE_PTR_HPP
#define CYPRESS_UTIL_CLONE_PTR_HPP

#include <algorithm>
#include <utility>

namespace cypress {

template <typename T>
class clone_ptr {
private:
	T *ptr;

	T *clone() const { return new T(*ptr); }

public:
	explicit clone_ptr(T *p = 0) : ptr(p) {}

	~clone_ptr() { delete ptr; }

	clone_ptr(const clone_ptr &p) : ptr(p.ptr ? p.clone() : 0) {}

	clone_ptr &operator=(const clone_ptr &p)
	{
		if (this != &p) {
			T *tmp = p.ptr ? p.clone() : 0;
			delete ptr;
			ptr = tmp;
		}
		return *this;
	}

	clone_ptr(clone_ptr &&p) noexcept : ptr(p.ptr) { p.ptr = 0; }

	clone_ptr &operator=(clone_ptr &&p)
	{
		std::swap(ptr, p.ptr);
		delete p.ptr;
		p.ptr = 0;
		return *this;
	}

	T *operator->() { return ptr; }
	const T *operator->() const { return ptr; }
	T &operator*() { return *ptr; }
	const T &operator*() const { return *ptr; }
};

template <typename T, typename... Args>
clone_ptr<T> make_clone(Args &&... args)
{
	return clone_ptr<T>(new T(std::forward<Args>(args)...));
}
}

#endif /* CYPRESS_UTIL_CLONE_PTR_HPP */
