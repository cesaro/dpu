#!/usr/bin/env python

import sys

def main () :
    try :
        sys.stdout.write ("__asm__ (\n")
        for s in sys.stdin :
            s = "%s" % s.rstrip()
            s = s.replace ("\\", "\\\\")
            s = s.replace ("\"", "\\\"")
            s = " \"%s\\n\"\n" % s
            sys.stdout.write (s)
        sys.stdout.write (");\n")

    except IOError :
        raise

if __name__ == '__main__' :
    main ()
