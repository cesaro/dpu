
#include <cstdlib>
#include <stdarg.h>
#include <memory>
#include <cstring>
#include <string>

#include "misc.hh"

std::string __attribute__((weak)) fmt (const std::string fmt_str, ...)
{
	/* reserve 2 times as much as the length of the fmt_str */
	int n = fmt_str.size() * 2;
	int final_n;
	std::string str;
	std::unique_ptr<char[]> formatted;

	va_list ap;
	while(1)
	{
		formatted.reset (new char[n]);
		strcpy (&formatted[0], fmt_str.c_str());
		va_start(ap, fmt_str);
		final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
		va_end(ap);
		if (final_n < 0 || final_n >= n)
			n += abs(final_n - n + 1);
		else
			break;
	}
	return std::string(formatted.get());
}

std::string __attribute__((weak)) operator * (std::string lhs, int i)
{
	int len;
	
	len = lhs.size ();
	lhs.reserve (len * i);
	for (; i >= 2; --i) lhs.append (lhs.data (), len);
	return lhs;
}

std::string __attribute__((weak)) quoted_str (const char * str)
{
	std::string s;

	s.reserve (strlen (str));
	for (const char * c = str; *c; ++c)
	{
		switch (*c)
		{
		case '\t' :
			s.append ("\\t");
			break;
		case '\r' :
			s.append ("\\r");
			break;
		case '\n' :
			s.append ("\\n");
			break;
		case '\0' : // unreachable
			s.append ("\\0");
			break;
		default :
			s.push_back (*c);
		}
	}
	return s;
}

