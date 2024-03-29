#!/usr/bin/bash
#
# Copyright (c) 2023 Robert Di Pardo
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
#
ENTITIES="$(dirname "$0")/HTMLTag-entities.ini"
EXCLUSIONS='(AMP|bne|bsol|colon|COPY|commat|DD|dollar|ENG|equals|ETH|excl|fjlig|GT|lpar|LT|NewLine|nvgt|nvlt|period|QUOT|quest|REG|rpar|sol|Tab|THORN|TRADE)'

cat<<-INI > $ENTITIES
[HTML 5]
; Generated by $(jq --version), $(date +'%Y-%m-%d %H:%M')
;
INI

curl -sL 'https://www.w3.org/TR/html5/entities.json' | jq --arg excl "$EXCLUSIONS" -r \
  '. as $ents | keys[] | select(index(";") and (test("^&\($excl);$") | not)) as $e | $ents[$e].codepoints | unique | map("\($e[1:-1])=\(tostring)")[]' \
  >> $ENTITIES

cat<<-INI >> $ENTITIES

[XML]
; C0 Controls and Basic Latin
quot=34			; quotation mark, U+0022 ISOnum
amp=38			; ampersand, U+0026 ISOnum
lt=60			; less-than sign, U+003C ISOnum
gt=62			; greater-than sign, U+003E ISOnum
apos=39			; apostrophe = APL quote, U+0027 ISOnum
INI
