#!/bin/bash

R="$1"

sed -i.sed "s/C.sar Rodr.guez/XXXX/" $R/README.rst
sed -i.sed "s/github.com.cesaro.dpu/XXXX/" $R/README.rst
sed -i.sed "s/lipn.univ-paris13.fr..rodriguez/XXXX/" $R/README.rst
sed -i.sed "s/our CONCUR'15 paper/the CONCUR'15 paper/" $R/README.rst

find $R | grep '\.sed$' | xargs rm

