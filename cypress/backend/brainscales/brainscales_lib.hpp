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
 * @fíle brainscales_wrap.hpp
 *
 * Defines a Singleton for dynamically loading the brainscales backend
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
 * @brief Singleton class for runtime loading of the BrainScaleS Backend, which
 * wraps all BrainScaleS libraries and will be provided pre-compiled.
 *
 */
class BS_Lib {
private:
	typedef Backend *create_bs(Json setup);
	typedef void destroy_bs(Backend *bck);
	void *m_lib;
	BS_Lib()
	{
		m_lib = dlopen("./libBS2CYPRESS.so", RTLD_LAZY);
		if (m_lib == NULL) {
			global_logger().debug(
			    "cypress",
			    "Error loading ./libBS2CYPRESS.so: " + std::string(dlerror()));
			global_logger().debug("cypress",
			                      "Installed bs lib will be used before "
			                      "subproject executable");
			m_lib = dlopen("libBS2CYPRESS.so",
			               RTLD_LAZY);  // if used in subproject
			if (m_lib == NULL) {
				global_logger().debug("cypress",
				                      "Error loading libBS2CYPRESS.so: " +
				                          std::string(dlerror()));
				m_lib = dlopen(BS_LIBRARY_PATH,
				               RTLD_LAZY);  // in LD_LIBRARY_PATH
				if (m_lib == NULL) {
					throw std::runtime_error(
					    "Error loading BrainScaleS backend: " +
					    std::string(dlerror()) +
					    "\nMake sure that you are running on a BrainScaleS "
					    "server and "
					    "bs2cypress lib is available either in this folder or "
					    "in LD_LIBRARY_PATH!");
				}
			}
		}
	};

	~BS_Lib() { dlclose(m_lib); }

public:
	BS_Lib(BS_Lib const &) = delete;
	void operator=(BS_Lib const &) = delete;

	/**
	 * @brief Creates a Singleton instance. Only at first time calling this
	 * method the library will be loaded.
	 *
	 * @return cypress::BS_Lib& reference to the library instance
	 */
	static BS_Lib &instance()
	{
		static BS_Lib instance;
		return instance;
	}

	/**
	 * @brief Creates a pointer to a BrainScaleS Backend. This can be executed
	 * several times, as it only creates a Backend pointer to a new created
	 * BrainScaleS Backend object.
	 *
	 * @param setup Setup config as defined by Backend constructor/ BrainScaleS
	 * constructor.
	 * @return pointer to the Backend object itself
	 */
	Backend *create_bs_backend(Json &setup)
	{
		create_bs *mkr = (create_bs *)dlsym(m_lib, "make_brainscales_backend");
		return mkr(setup);
	}
};
}  // namespace cypress
