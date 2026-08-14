#!/usr/bin/env bash
# Teste le parsing des fichiers de conf : conf/*.conf doivent etre acceptes,
# conf/bad/*.conf doivent etre rejetes (exit code != 0).

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/webserv"
GOOD_DIR="$ROOT_DIR/conf"
BAD_DIR="$ROOT_DIR/conf/bad"
VALGRIND_LOG_DIR="/tmp/test_conf_valgrind_logs"

# Passe --no-valgrind pour sauter le check memoire (plus rapide)
CHECK_LEAKS=1
[ "${1:-}" = "--no-valgrind" ] && CHECK_LEAKS=0

pass=0
fail=0
leaks=0

check_leaks() {
	local file="$1"
	local log="$VALGRIND_LOG_DIR/$(basename "$file").log"

	valgrind --leak-check=full --track-fds=yes --error-exitcode=99 --quiet \
		"$BIN" "$file" > /dev/null 2> "$log"
	local status=$?

	# valgrind ne compte pas les fd ouverts comme des erreurs : il faut lire
	# le rapport a la main. On ignore 0/1/2 (stdin/stdout/stderr).
	local fds
	fds=$(grep -c "Open file descriptor [3-9]" "$log")

	if [ "$status" -eq 99 ] || grep -q "definitely lost: [1-9]" "$log" \
		|| grep -q "indirectly lost: [1-9]" "$log"; then
		echo "      -> LEAK memoire detecte (voir $log)"
		leaks=$((leaks + 1))
		return 1
	fi
	if [ "$fds" -gt 0 ]; then
		echo "      -> $fds FD non ferme(s) (voir $log)"
		leaks=$((leaks + 1))
		return 1
	fi
	rm -f "$log"
	return 0
}

run_case() {
	local file="$1"
	local expect="$2" # "good" ou "bad"

	timeout 2 "$BIN" "$file" > /tmp/test_conf_out.log 2>&1
	local status=$?

	if [ "$expect" = "good" ]; then
		if [ "$status" -eq 0 ]; then
			echo "PASS  (accepte)  $file"
			pass=$((pass + 1))
		else
			echo "FAIL  (rejete alors qu'il devrait passer)  $file"
			fail=$((fail + 1))
		fi
	else
		if [ "$status" -ne 0 ]; then
			echo "PASS  (rejete)   $file"
			pass=$((pass + 1))
		else
			echo "FAIL  (accepte alors qu'il devrait etre rejete)  $file"
			fail=$((fail + 1))
		fi
	fi

	[ "$CHECK_LEAKS" -eq 1 ] && check_leaks "$file"
}

if [ ! -x "$BIN" ]; then
	echo "Binaire introuvable ou non executable : $BIN (fais 'make' d'abord)"
	exit 1
fi

[ "$CHECK_LEAKS" -eq 1 ] && mkdir -p "$VALGRIND_LOG_DIR"

echo "=== Fichiers valides (conf/*.conf) ==="
for file in "$GOOD_DIR"/*.conf; do
	run_case "$file" "good"
done

echo
echo "=== Fichiers invalides (conf/bad/*.conf) ==="
for file in "$BAD_DIR"/*.conf; do
	run_case "$file" "bad"
done

echo
echo "=== Resume : $pass reussi(s), $fail echoue(s), $leaks leak(s) ==="
[ "$leaks" -eq 0 ] && rmdir "$VALGRIND_LOG_DIR" 2>/dev/null
rm -f /tmp/test_conf_out.log

[ "$fail" -eq 0 ] && [ "$leaks" -eq 0 ]
