/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2019  Christoph Ostrau
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

/**
 * @fíle genn_lib.hpp
 *
 * Defines a Singleton for dynamically loading the genn backend
 *
 * @author Christoph Ostrau
 */

#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <dlfcn.h>

#include <cypress/core/backend.hpp>
#include <cypress/util/json.hpp>
#include <cypress/util/logger.hpp>

namespace cypress {
/**
 * @brief Singleton class for runtime loading of the GeNN Backend, which
 * wraps all GeNN/GPU libraries
 *
 */
class GENN_Lib {
private:
	typedef Backend *create_bs(Json setup);
	typedef void destroy_bs(Backend *bck);
	void *m_lib;
	GENN_Lib()
	{
		m_lib = dlopen("./libgennbck.so", RTLD_LAZY);
		if (m_lib == NULL) {
            global_logger().debug("cypress",
		                      "Installed GeNN lib will be used before "
		                      "subproject executable");
			m_lib = dlopen("libgennbck.so",
			               RTLD_LAZY);  // if used in subproject
			if (m_lib == NULL) {
                m_lib = dlopen(GENN_LIBRARY_PATH,
				               RTLD_LAZY);  // in LD_LIBRARY_PATH
				if (m_lib == NULL) {
					throw std::runtime_error(
					    "Error loading GeNN backend: " +
					    std::string(dlerror()) +
					    "\nMake sure that "
					    "libgennbck lib is available either in this folder or "
					    "in LD_LIBRARY_PATH!");
				}
			}
		}
	};

	~GENN_Lib() { dlclose(m_lib); }

public:
	GENN_Lib(GENN_Lib const &) = delete;
	void operator=(GENN_Lib const &) = delete;

	/**
	 * @brief Creates a Singleton instance. Only at first time calling this
	 * method the library will be loaded.
	 *
	 * @return cypress::GENN_Lib& reference to the library instance
	 */
	static GENN_Lib &instance()
	{
		static GENN_Lib instance;
		return instance;
	}

	/**
	 * @brief Creates a pointer to a GeNN Backend. This can be executed
	 * several times, as it only creates a Backend pointer to a new created
	 * GeNN Backend object.
	 *
	 * @param setup Setup config as defined by Backend constructor/ GeNN
	 * constructor.
	 * @return pointer to the Backend object itself
	 */
	Backend *create_genn_backend(Json &setup)
	{
		create_bs *mkr = (create_bs *)dlsym(m_lib, "make_genn_backend");
		return mkr(setup);
	}
};
}  // namespace cypress
