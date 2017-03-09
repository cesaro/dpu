#!/usr/bin/env python

import sys

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

def val (v) :
   if is_numeric (v) :
      return v
   if is_quoted (v) :
      return v
   return '"%s"' % v

def error (l, m) :
   print '// error: "%s": %s' % (l, m)

def main () :
   print '// Automatically generated using mkh.py, do not edit!'
   print '#ifndef _CONFIG__'
   print '#define _CONFIG__'

   for l in sys.stdin :
      l = l.strip()
      if len (l) == 0 :
         print
         continue
      if l[0] == "#" :
         print '//' + l[1:]
         continue
      if ':=' in l :
         tok = l.split (':=')
      elif '?=' in l :
         tok = l.split ('?=')
      elif '=' in l :
         tok = l.split ('=')
      else :
         error (l, 'unable to parse as a variable declaration')
         sys.exit (1)

      print '#define', tok[0].strip(), val(tok[1].strip())

   print '#endif'

if __name__ == '__main__' :
    main ()
