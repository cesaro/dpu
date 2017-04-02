#!/usr/bin/env python

# This scripts expects to read from the standard input a list of \n separated
# integers in base 16 (adresses). It will then print the number of unique
# integers it has read and the number of contiguous intervals (regions) that
# those integers can be grouped into.

import sys

ls = []
uniq = {}
total = 0

print 'readin stdin ...'
for l in sys.stdin :
   total += 1
   a = int (l, 16)

   #a = 8 * (a / 8) # sort of equivalent to the job done by variable w below

   if a in uniq :
      uniq[a] += 1
   else :
      uniq[a] = 1
   ls.append (a)
   if total % 1000 == 0 :
      print 'addresses', total

w = 8 # width of a memory operation (bytes)

print 'sorting ...'
sor = sorted (ls)
regions = {}
l = sor[0]
h = l
count = 1

print 'clustering ...'
for a in sor[1:] :
   # if the new address is <= than the last byte written by the last write +1,
   # then we are still in the same region
   if a <= h + w :
      count += 1
      h = max (a, h)
      continue
   regions[l,h] = count
   l = a
   h = a
   count = 1
   if len (regions) % 1000 == 0 :
      print 'regions', len (regions)

regions[l,h] = count

last = 0
for l,h in sorted (regions, key =lambda (l,h): l) :
   print '+', l - last
   last = h
   print l, '-', h, ':', regions[l,h], 'addr', h - l, 'b'

print 'addresses', total
print 'unique addresses', len (uniq)
print 'regions', len (regions)

