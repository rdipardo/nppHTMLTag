#!/usr/bin/bash
#
# Copyright (c) 2023,2024 Robert Di Pardo
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
#
test -z "$BB_REPO_TOKEN" && exit 0
set -e
cd "$BIN_DIR"

curl -sJLO 'https://bitbucket.org/rdipardo/htmltag/downloads/sha256sums.txt'
sha256sum "${SLUGX86}" >> sha256sums.txt
sha256sum "${SLUGX64}" >> sha256sums.txt
sha256sum "${SLUGARM64}" >> sha256sums.txt

curl --request POST \
  --url "https://api.bitbucket.org/2.0/repositories/${CIRCLE_USERNAME}/${CIRCLE_PROJECT_REPONAME}/downloads" \
  --header "Authorization: Bearer ${BB_REPO_TOKEN}" \
   -F files=@"${SLUGX86}" -F files=@"${SLUGX64}" -F files=@"${SLUGARM64}"

curl --request POST \
  --url "https://api.bitbucket.org/2.0/repositories/${CIRCLE_USERNAME}/${CIRCLE_PROJECT_REPONAME}/downloads" \
  --header "Authorization: Bearer ${BB_REPO_TOKEN}" \
   -F files=@'sha256sums.txt'
