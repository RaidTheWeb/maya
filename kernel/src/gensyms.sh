#! /bin/sh

set -e

LC_ALL=C
export LC_ALL

TMP0="$(mktemp)"

cat >"$TMP0" <<EOF
#! /bin/sh

set -e

set -o pipefail 2>/dev/null
EOF

chmod +x "$TMP0"

"$TMP0" && set -o pipefail

rm "$TMP0"

TMP1="$(mktemp)"
TMP2="$(mktemp)"
TMP3="$(mktemp)"

trap "rm -f '$TMP1' '$TMP2' '$TMP3'; trap - EXIT; exit" EXIT INT TERM QUIT HUP

objdump -t "$1" | ( sed '/[[:<:]]d[[:>:]]/d' 2>/dev/null || sed '/\bd\b/d' ) | sort > "$TMP1"
grep "\.text" < "$TMP1" | cut -d' ' -f1 > "$TMP2"
grep "\.text" < "$TMP1" | "awk" 'NF{ print $NF }' > "$TMP3"

cat <<EOF
#include <stdint.h>
#include <trace.h>

symbol_t symboltable[] = {
EOF

paste -d'$' "$TMP2" "$TMP3" | sed "s/^/    {0x/g;s/\\\$/, \"/g;s/\$/\"},/g"

cat <<EOF
    {0xffffffffffffffff, ""}
};
EOF
