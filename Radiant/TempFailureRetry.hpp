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
