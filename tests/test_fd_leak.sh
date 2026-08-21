#!/usr/bin/env bash
# Teste la fermeture des connexions (B-04) : aucun fd ne doit fuir, quel que
# soit le chemin de sortie du client.
#
# Pourquoi ce script plutot que la boucle curl de la DoD : 1000 `curl` forkes
# ne se chevauchent pas (fork+exec complet a chaque fois), le pic reel tourne
# autour de 4 connexions simultanees. Ici on ouvre de vrais sockets, on les
# garde, et on coupe brutalement avec SO_LINGER {on=1, timeout=0} -> RST.
#
# Le script demarre sa PROPRE instance et retient son PID via $! : pas de
# `pidof webserv`, qui renvoie plusieurs PID des que deux instances tournent
# et fait mesurer la mauvaise.
#
# Usage :
#   ./tests/test_fd_leak.sh              # tout
#   ./tests/test_fd_leak.sh --port 9500  # si 8099 est pris
#   ./tests/test_fd_leak.sh --keep       # laisse le serveur tourner a la fin

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/webserv"
PORT=8099
KEEP=0

while [ $# -gt 0 ]; do
	case "$1" in
		--port) PORT="$2"; shift 2 ;;
		--keep) KEEP=1; shift ;;
		*) echo "usage: $0 [--port N] [--keep]"; exit 2 ;;
	esac
done

[ -x "$BIN" ] || { echo "webserv introuvable, lance 'make' d'abord"; exit 2; }
command -v python3 >/dev/null || { echo "python3 requis"; exit 2; }

CONF="$(mktemp /tmp/fdleak_XXXXXX.conf)"
LOG="$(mktemp /tmp/fdleak_XXXXXX.log)"
printf 'server\n{\n\tlisten %s;\n}\n' "$PORT" > "$CONF"

cleanup() {
	if [ "$KEEP" = "0" ] && [ -n "${SRV_PID:-}" ] && kill -0 "$SRV_PID" 2>/dev/null; then
		kill "$SRV_PID" 2>/dev/null
		wait "$SRV_PID" 2>/dev/null
	fi
	rm -f "$CONF"
	[ "$KEEP" = "0" ] && rm -f "$LOG"
}
trap cleanup EXIT

"$BIN" "$CONF" > "$LOG" 2>&1 &
SRV_PID=$!
python3 -c 'import time; time.sleep(0.5)'

if ! kill -0 "$SRV_PID" 2>/dev/null; then
	echo "le serveur n'a pas demarre sur le port $PORT :"
	cat "$LOG"
	echo "(relance avec --port <autre> si le port est occupe)"
	exit 2
fi

echo "webserv PID=$SRV_PID  port=$PORT"
echo

PORT="$PORT" python3 - "$SRV_PID" <<'PYEOF'
import os, socket, struct, sys, time

pid  = sys.argv[1]
PORT = int(os.environ["PORT"])
HOST = "127.0.0.1"

def fds():
	return len(os.listdir("/proc/%s/fd" % pid))

def rst(s):
	"""close() qui envoie un RST : coupure brutale, pas de FIN poli."""
	s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
	s.close()

def conn():
	return socket.create_connection((HOST, PORT), timeout=5)

fails = []

def run(tag, n, prime=None, brutal=True, hold=0.3):
	before = fds()
	socks = []
	try:
		for _ in range(n):
			s = conn()
			if prime:
				prime(s)
			socks.append(s)
	except Exception as e:
		print("  %-36s ERREUR a l'ouverture: %s" % (tag, e))
		fails.append(tag)
		for s in socks:
			s.close()
		return
	time.sleep(hold)
	peak = fds()
	for s in socks:
		rst(s) if brutal else s.close()
	time.sleep(1.0)
	after = fds()
	ok = (after == before)
	print("  %-36s avant=%-4d pic=%-4d apres=%-4d  %s"
	      % (tag, before, peak, after,
	         "OK" if ok else ">>> FUITE +%d <<<" % (after - before)))
	if not ok:
		fails.append(tag)

def hdr(s):      s.sendall(b"GET / HTTP/1.1\r\nHost: x\r\n\r\n")
def partial(s):  s.sendall(b"GET / HTTP/1.1\r\nHost: local")
def upload(s):   s.sendall(b"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 100000\r\n\r\n" + b"A" * 500)
def pipeline(s): s.sendall(b"GET / HTTP/1.1\r\nHost: x\r\n\r\n" * 10)
def finwr(s):
	hdr(s)
	s.shutdown(socket.SHUT_WR)   # FIN propre -> recv() renvoie 0 cote serveur

base = fds()
print("baseline = %d fds (0,1,2 + 1 socket d'ecoute)\n" % base)

print("[ fuites de fd ]")
run("100 conn ouvertes puis RST",     100)
run("400 conn ouvertes puis RST",     400)
run("headers partiels puis RST",      200, partial)
run("upload coupe en plein body",     200, upload)
run("RST avant de lire la reponse",   200, hdr, hold=0.0)
run("shutdown(WR) propre",            200, finwr, brutal=False, hold=0.5)
run("pipelining x10 puis RST",        100, pipeline)

# --- Informatif : timeout d'inactivite (B-05, pas encore implemente) --------
print("\n[ timeout d'inactivite - B-05 ]")
before = fds()
socks = [conn() for _ in range(50)]
for s in socks:
	s.sendall(b"GET / HTTP/1.1\r\n")   # headers jamais termines : slowloris
time.sleep(12)
held = fds()
for s in socks:
	s.close()
time.sleep(1)
if held > before:
	print("  50 conn inactives, +12s          : %d fds toujours ouverts (attendu tant que B-05 n'est pas fait)" % (held - before))
else:
	print("  50 conn inactives, +12s          : recyclees, B-05 est en place")

print("\nfds final = %d" % fds())
if fails:
	print("\nECHEC sur %d cas : %s" % (len(fails), ", ".join(fails)))
	sys.exit(1)
print("\nTous les cas de fermeture passent : aucun fd ne fuit.")
PYEOF

RC=$?
echo
if kill -0 "$SRV_PID" 2>/dev/null; then
	echo "serveur toujours vivant apres le stress : pas de crash."
else
	echo ">>> le serveur est MORT pendant le test <<<"
	cat "$LOG"
	RC=1
fi
[ "$KEEP" = "1" ] && echo "serveur laisse en vie (PID $SRV_PID), log: $LOG"
exit $RC
