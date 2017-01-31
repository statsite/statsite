#
# Check: a unit test framework for C
# Copyright (C) 2011 Mateusz Loskot
# Copyright (C) 2001, 2002 Arien Malec
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

set(SOURCES
  check.c
  check_error.c
  check_list.c
  check_log.c
  check_msg.c
  check_pack.c
  check_print.c
  check_run.c
  check_str.c)

set(HEADERS 
  ${CONFIG_HEADER}
  ${CMAKE_CURRENT_BINARY_DIR}/check.h
  check.h.in
  check_error.h
  check_impl.h
  check_list.h
  check_log.h
  check_msg.h
  check_pack.h
  check_print.h
  check_str.h)

configure_file(check.h.in check.h)

# Enable finding check.h
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(check STATIC ${SOURCES} ${HEADERS})
target_link_libraries(check ${LIBM} ${LIBRT} ${SUBUNIT})

if(MSVC)
  add_definitions(-DCK_DLL_EXP=_declspec\(dllexport\))
endif (MSVC)

install(TARGETS check 
  EXPORT check
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(FILES ${CMAKE_BINARY_DIR}/src/check.h DESTINATION include)
install(EXPORT check DESTINATION cmake)
