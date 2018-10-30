#!/bin/bash -x

rpmbuild --define "_srcrpmdir `cd ..; pwd`" \
 -bs --build-in-place cinelerra.spec

