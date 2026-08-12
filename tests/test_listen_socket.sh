#!/usr/bin/env bash
# Teste le comportement de ListenSockets (B-01) au niveau socket, pas au
# niveau parsing (voir test_conf.sh pour ca) :
#   - cas nominal : les .conf valides sur des ports libres doivent demarrer.
#   - port deja occupe : bind() doit echouer proprement (exit != 0), pas planter.
#   - redemarrage rapide : SO_REUSEADDR doit permettre de relancer aussitot.
#
# Limite connue : ce script est un test boite noire (code de sortie / duree
# de vie du process). Il ne verifie pas les flags bas niveau (O_NONBLOCK sur
# le fd, SO_REUSEADDR effectivement pose) - ca se verifie plutot avec un
# getsockopt()/fcntl(F_GETFL) de debug directement dans le code, voir la
# discussion sur ListenSockets.cpp.

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/webserv"
GOOD_DIR="$ROOT_DIR/conf"
VALGRIND_LOG_DIR="/tmp/test_listen_socket_valgrind_logs"

# .conf valides mais qui ne dependent pas d'un port privilegie (<1024) :
# default.conf est exclu ici expres, il resout "listen 127.0.0.1;" en
# 127.0.0.1:80 (voir docs), qui necessite root et n'est pas ce qu'on teste.
NOMINAL_FILES=(
	"$GOOD_DIR/minimal.conf"
	"$GOOD_DIR/tester.conf"
	"$GOOD_DIR/test_resolve.conf"
	"$GOOD_DIR/multi_server_no_name.conf"
)

# Passe --no-valgrind pour sauter le check memoire (plus rapide)
CHECK_LEAKS=1
[ "${1:-}" = "--no-valgrind" ] && CHECK_LEAKS=0

pass=0
fail=0
leaks=0

check_leaks() {
	local file="$1"
	local log="$VALGRIND_LOG_DIR/$(basename "$file").log"

	valgrind --leak-check=full --error-exitcode=99 --quiet \
		"$BIN" "$file" > /dev/null 2> "$log"

	if [ "$?" -eq 99 ] || grep -q "definitely lost: [1-9]" "$log" \
		|| grep -q "indirectly lost: [1-9]" "$log"; then
		echo "      -> LEAK detecte (voir $log)"
		leaks=$((leaks + 1))
		return 1
	fi
	rm -f "$log"
	return 0
}

if [ ! -x "$BIN" ]; then
	echo "Binaire introuvable ou non executable : $BIN (fais 'make' d'abord)"
	exit 1
fi

[ "$CHECK_LEAKS" -eq 1 ] && mkdir -p "$VALGRIND_LOG_DIR"

echo "=== Cas nominal : sockets sur ports libres ==="
for file in "${NOMINAL_FILES[@]}"; do
	timeout 2 "$BIN" "$file" > /tmp/test_listen_socket_out.log 2>&1
	status=$?

	if [ "$status" -eq 0 ]; then
		echo "PASS  (sockets ouverts)  $file"
		pass=$((pass + 1))
	else
		echo "FAIL  (rejete alors qu'il devrait passer)  $file"
		cat /tmp/test_listen_socket_out.log
		fail=$((fail + 1))
	fi

	[ "$CHECK_LEAKS" -eq 1 ] && check_leaks "$file"
done

echo
echo "=== Port deja occupe : bind() doit echouer proprement ==="
OCCUPIED_PORT=8080
OCCUPIED_CONF="$GOOD_DIR/minimal.conf"

nc -l "$OCCUPIED_PORT" > /dev/null 2>&1 &
NC_PID=$!
sleep 0.2 # laisse nc le temps de bind avant qu'on lance webserv dessus

timeout 2 "$BIN" "$OCCUPIED_CONF" > /tmp/test_listen_socket_out.log 2>&1
status=$?

kill "$NC_PID" 2>/dev/null
wait "$NC_PID" 2>/dev/null

if [ "$status" -ne 0 ]; then
	echo "PASS  (echec propre, port $OCCUPIED_PORT occupe)  $OCCUPIED_CONF"
	pass=$((pass + 1))
else
	echo "FAIL  (aurait du echouer, port $OCCUPIED_PORT etait occupe)  $OCCUPIED_CONF"
	fail=$((fail + 1))
fi

echo
echo "=== Redemarrage rapide : SO_REUSEADDR ne doit pas bloquer un relaunch ==="
# NB : ce test ne prouve pas a lui seul l'effet de SO_REUSEADDR (il faudrait
# une vraie connexion etablie puis coupee pour generer un TIME_WAIT), mais
# sert de non-regression : deux lancements consecutifs doivent tous les deux
# reussir.
RESTART_CONF="$GOOD_DIR/minimal.conf"
restart_ok=1

for i in 1 2 3; do
	timeout 2 "$BIN" "$RESTART_CONF" > /tmp/test_listen_socket_out.log 2>&1
	if [ "$?" -ne 0 ]; then
		restart_ok=0
	fi
done

if [ "$restart_ok" -eq 1 ]; then
	echo "PASS  (3 lancements consecutifs OK)  $RESTART_CONF"
	pass=$((pass + 1))
else
	echo "FAIL  (un des 3 lancements consecutifs a echoue)  $RESTART_CONF"
	cat /tmp/test_listen_socket_out.log
	fail=$((fail + 1))
fi

echo
echo "=== Resume : $pass reussi(s), $fail echoue(s), $leaks leak(s) ==="
[ "$leaks" -eq 0 ] && rmdir "$VALGRIND_LOG_DIR" 2>/dev/null
rm -f /tmp/test_listen_socket_out.log

[ "$fail" -eq 0 ] && [ "$leaks" -eq 0 ]
