
#include <cstdlib>
#include <stdarg.h>
#include <memory>
#include <cstring>
#include <string>

#include "misc.hh"

std::string fmt (const std::string fmt_str, ...)
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

