#!/usr/bin/env bash
# Teste Resolve() et build_path() : chaque cas donne une URI, la location
# attendue et le chemin disque attendu, resolus sur conf/test_resolve.conf.

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/webserv"
CONF="$ROOT_DIR/conf/test_resolve.conf"
VALGRIND_LOG_DIR="/tmp/test_resolve_valgrind_logs"

# Passe --no-valgrind pour sauter le check memoire (plus rapide)
CHECK_LEAKS=1
[ "${1:-}" = "--no-valgrind" ] && CHECK_LEAKS=0

pass=0
fail=0
leaks=0

check_leaks() {
	local uri="$1"
	local log="$VALGRIND_LOG_DIR/$(echo "$uri" | tr -c 'a-zA-Z0-9' '_').log"

	valgrind --leak-check=full --error-exitcode=99 --quiet \
		"$BIN" "$CONF" --resolve "$uri" > /dev/null 2> "$log"

	if [ "$?" -eq 99 ] || grep -q "definitely lost: [1-9]" "$log" \
		|| grep -q "indirectly lost: [1-9]" "$log"; then
		echo "        -> LEAK detecte (voir $log)"
		leaks=$((leaks + 1))
		return 1
	fi
	rm -f "$log"
	return 0
}

run_case() {
	local uri="$1" want_loc="$2" want_path="$3"

	local out
	out="$(timeout 2 "$BIN" "$CONF" --resolve "$uri" 2>&1)"

	local got_loc got_path
	got_loc="$(printf '%s\n' "$out"  | sed -n 's/^location: //p')"
	got_path="$(printf '%s\n' "$out" | sed -n 's/^path: //p')"

	if [ "$got_loc" = "$want_loc" ] && [ "$got_path" = "$want_path" ]; then
		printf 'PASS  %-28s -> %s\n' "$uri" "$got_path"
		pass=$((pass + 1))
	else
		printf 'FAIL  %s\n' "$uri"
		printf '        location : %-14s (attendu %s)\n' "$got_loc"  "$want_loc"
		printf '        path     : %-14s (attendu %s)\n' "$got_path" "$want_path"
		fail=$((fail + 1))
	fi

	[ "$CHECK_LEAKS" -eq 1 ] && check_leaks "$uri"
}

if [ ! -x "$BIN" ]; then
	echo "Binaire introuvable ou non executable : $BIN (fais 'make' d'abord)"
	exit 1
fi

[ "$CHECK_LEAKS" -eq 1 ] && mkdir -p "$VALGRIND_LOG_DIR"

echo "=== Resolve() + build_path() sur conf/test_resolve.conf ==="
#        URI                          location    chemin attendu
run_case "/kapouet/pouic/toto/pouet"   "/kapouet"  "/tmp/www/pouic/toto/pouet"
run_case "/kapouet"                    "/kapouet"  "/tmp/www"
run_case "/kapouet/"                   "/kapouet"  "/tmp/www/"
run_case "/kapouetXX"                  "/"         "./www/kapouetXX"
run_case "/kap"                        "/kap"      "/tmp/kap"
run_case "/kap/toto"                   "/kap"      "/tmp/kap/toto"
run_case "/kapouet?id=42"              "/kapouet"  "/tmp/www"
run_case "/kapouet/pouic#ancre"        "/kapouet"  "/tmp/www/pouic"
run_case "/images/logo.png"            "/images/"  "/tmp/img/logo.png"
run_case "/images"                     "/"         "./www/images"
run_case "/"                           "/"         "./www"
run_case "/rien/du/tout"               "/"         "./www/rien/du/tout"

echo
echo "=== Resume : $pass reussi(s), $fail echoue(s), $leaks leak(s) ==="
[ "$leaks" -eq 0 ] && rmdir "$VALGRIND_LOG_DIR" 2>/dev/null

[ "$fail" -eq 0 ] && [ "$leaks" -eq 0 ]
