#!/bin/bash
( ./autogen.sh
  ./configure --with-single-user
  make && make install ) 2>&1 | tee log
mv Makefile Makefile.cfg
cp Makefile.devel Makefile
