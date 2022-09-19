/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef TEMPFAILURERETRY_HPP
#define TEMPFAILURERETRY_HPP

#ifndef __linux__
#define TEMP_FAILURE_RETRY(expr) \
	({ long int _res; \
	 do _res = (long int) (expr); \
	 while (_res == -1L && errno == EINTR); \
	 _res; })
#endif

#endif
