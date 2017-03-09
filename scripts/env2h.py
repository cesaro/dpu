#!/usr/bin/env python

import sys
import os

def is_numeric (s) :
   for c in s :
      if c not in " 0123456789()+-*/%<.>" :
         return False
   return True

   try :
      x = float (s)
      return True
   except ValueError :
      return False

def is_quoted (s) :
   return len (s) and s[0] == '"' and s[-1] == '"'

def quote (v) :
   if is_numeric (v) :
      return v
   if is_quoted (v) :
      return v
   return '"%s"' % v

def main () :

   print '// Automatically generated using env2h.py, do not edit!'
   print '#ifndef _CONFIG__'
   print '#define _CONFIG__'
   print

   varset = set (sys.argv[1:])
   for var in varset :
      if var not in os.environ :
         print '#undef', var
         continue
      val = os.environ[var]
      val = val.strip()
      print '#define', var, quote(val)

   print
   print '#endif'

if __name__ == '__main__' :
    main ()
