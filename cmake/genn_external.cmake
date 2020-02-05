#  Cypress -- C++ Spiking Neural Network Simulation Framework
#  Copyright (C) 2019  Christoph Ostrau
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

include(ExternalProject)


include(CheckLanguage)
check_language(CUDA)
find_package(CUDA)

ExternalProject_Add(genn_ext
        GIT_REPOSITORY        "https://github.com/genn-team/genn.git"
        GIT_TAG               686cbd968ad7d267e856fe8f25adf04352bb64a3
        CONFIGURE_COMMAND     ""
        CMAKE_COMMAND         ""
        BUILD_COMMAND         CXXFLAGS=-fPIC make -j -C <SOURCE_DIR>
        INSTALL_COMMAND 	  ""
        UPDATE_COMMAND        ""
        EXCLUDE_FROM_ALL      TRUE
        BUILD_ALWAYS          FALSE
    )

ExternalProject_Get_Property(genn_ext SOURCE_DIR BINARY_DIR)
set(GENN_INCLUDE_DIRS
    ${SOURCE_DIR}/include/
    ${SOURCE_DIR}/include/genn/genn
    ${SOURCE_DIR}/include/genn/third_party/
    ${SOURCE_DIR}/userproject/include/
)
include_directories(${GENN_INCLUDE_DIRS})

if((DEFINED ENV{CUDA_PATH}) OR (EXISTS ${SOURCE_DIR}/lib/libgenn_cuda_backend.a))
    include_directories(${CUDA_INCLUDE_DIRS} ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})
    target_compile_definitions(gennbck PRIVATE CUDA_PATH_DEFINED)
    set(GENN_LIBRARIES
        ${SOURCE_DIR}/lib/libgenn_cuda_backend.a
        ${SOURCE_DIR}/lib/libgenn_single_threaded_cpu_backend.a
        ${SOURCE_DIR}/lib/libgenn.a
        -L${CUDA_TOOLKIT_ROOT_DIR}/lib64
        cuda
        cudart
    )
else()
    set(GENN_LIBRARIES
        ${SOURCE_DIR}/lib/libgenn_single_threaded_cpu_backend.a
        ${SOURCE_DIR}/lib/libgenn.a
    )
endif()

