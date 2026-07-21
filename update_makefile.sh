#!/usr/bin/env bash
# Regenerates the SRCS_* block in the Makefile from the contents of srcs/.
# Each top-level file in srcs/ goes into SRCS_MAIN; each subdirectory
# srcs/<name>/ becomes SRCS_<NAME> with its .cpp files.

set -eo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

SRC_DIR="srcs"
MAKEFILE="Makefile"
START_MARKER="# >>> SRCS AUTO-GENERATED >>>"
END_MARKER="# <<< SRCS AUTO-GENERATED <<<"

[ -d "$SRC_DIR" ] || { echo "error: $SRC_DIR/ not found" >&2; exit 1; }
[ -f "$MAKEFILE" ] || { echo "error: $MAKEFILE not found" >&2; exit 1; }

if ! grep -qF "$START_MARKER" "$MAKEFILE" || ! grep -qF "$END_MARKER" "$MAKEFILE"; then
	echo "error: markers '$START_MARKER' / '$END_MARKER' not found in $MAKEFILE" >&2
	exit 1
fi

tmp_block="$(mktemp)"
trap 'rm -f "$tmp_block"' EXIT

var_names=()

# Appends one SRCS_<NAME> block. Args: varname, dir-prefix (may be empty), files...
emit_group() {
	local varname="$1" prefix="$2"
	shift 2
	local files=("$@")
	[ "${#files[@]}" -eq 0 ] && return
	var_names+=("$varname")
	printf '%s = $(addprefix $(SRC_DIR)/%s, \\\n' "$varname" "$prefix" >> "$tmp_block"
	local last=$(( ${#files[@]} - 1 ))
	local i
	for i in "${!files[@]}"; do
		if [ "$i" -eq "$last" ]; then
			printf '\t%s)\n\n' "${files[$i]}" >> "$tmp_block"
		else
			printf '\t%s \\\n' "${files[$i]}" >> "$tmp_block"
		fi
	done
}

mapfile -t root_files < <(find "$SRC_DIR" -maxdepth 1 -type f -name '*.cpp' -printf '%f\n' | sort)
if [ "${#root_files[@]}" -gt 0 ]; then
	emit_group "SRCS_MAIN" "" "${root_files[@]}"
fi

mapfile -t subdirs < <(find "$SRC_DIR" -mindepth 1 -maxdepth 1 -type d -printf '%f\n' | sort)
for dir in "${subdirs[@]}"; do
	mapfile -t files < <(find "$SRC_DIR/$dir" -maxdepth 1 -type f -name '*.cpp' -printf '%f\n' | sort)
	if [ "${#files[@]}" -gt 0 ]; then
		varname="SRCS_$(printf '%s' "$dir" | tr '[:lower:]' '[:upper:]')"
		emit_group "$varname" "$dir/" "${files[@]}"
	fi
done

if [ "${#var_names[@]}" -eq 0 ]; then
	echo "error: no .cpp files found under $SRC_DIR/" >&2
	exit 1
fi

{
	echo "# Source files"
	list=""
	for v in "${var_names[@]}"; do
		list+="\$($v) "
	done
	printf 'SRCS = %s\n' "${list% }"
} >> "$tmp_block"

awk -v startm="$START_MARKER" -v endm="$END_MARKER" -v blockfile="$tmp_block" '
	BEGIN { while ((getline line < blockfile) > 0) block = block line "\n" }
	$0 == startm { print; printf "%s", block; skip=1; next }
	$0 == endm   { print; skip=0; next }
	skip { next }
	{ print }
' "$MAKEFILE" > "$MAKEFILE.tmp" && mv "$MAKEFILE.tmp" "$MAKEFILE"

echo "Makefile updated: ${var_names[*]}"
