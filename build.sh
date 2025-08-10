#!/bin/sh
sed -e "s/\${GIT_SHA}/$(git rev-parse --short HEAD)/g" \
    -e "s/\${BUILD_DATE}/$(date)/g" \
    crane.html > index.html