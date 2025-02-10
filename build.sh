#!/bin/sh
sed "s/\${GIT_SHA}/$(git rev-parse --short HEAD)/g" crane.html > index.html