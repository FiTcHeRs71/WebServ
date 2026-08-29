#!/usr/bin/env bash
# Teste le cycle de vie des processus CGI (D-05) : recolte, timeout, et
# nettoyage sur tous les chemins de sortie.
#
# Les trois bugs que ce script verrouille, dans l'ordre ou ils ont ete trouves :
#
#   1. COURSE A LA RECOLTE. Quand le CGI ferme stdout avant d'avoir ete
#      waitpid(), l'ancien code faisait un `return` sec : le pipe etant deja
#      clos, plus aucun evenement poll() ne revenait dessus et le client
#      n'obtenait JAMAIS de reponse. ~20% des requetes partaient en timeout.
#      Le bug est invisible sous valgrind (tout est ralenti, l'enfant a
#      toujours fini a temps) : il FAUT le test 1 en vitesse reelle.
#
#   2. DOUBLE REPONSE. Un CGI expire etait Kill() par SweepTimeouts (-> 504)
#      puis re-recolte par SweepPendingReap (-> 200) : deux reponses HTTP
#      dans la meme connexion. curl n'y voit que du feu, il s'arrete au
#      Content-Length -- d'ou la lecture brute au socket du test 4.
#
#   3. USE-AFTER-FREE. CloseConnection() faisait _Clients.erase() AVANT de
#      lire l'iterateur pour tuer le CGI. Invisible fonctionnellement, la
#      memoire libere restant intacte : seul --valgrind le voit.
#
# Usage :
#   ./tests/test_cgi_lifecycle.sh              # tout, en vitesse reelle
#   ./tests/test_cgi_lifecycle.sh --port 9500  # si 8099 est pris
#   ./tests/test_cgi_lifecycle.sh --valgrind   # ajoute le controle memoire/fd
#   ./tests/test_cgi_lifecycle.sh --keep       # laisse le serveur tourner

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/webserv"
PORT=8099
KEEP=0
VG=0

while [ $# -gt 0 ]; do
	case "$1" in
		--port) PORT="$2"; shift 2 ;;
		--keep) KEEP=1; shift ;;
		--valgrind) VG=1; shift ;;
		*) echo "usage: $0 [--port N] [--keep] [--valgrind]"; exit 2 ;;
	esac
done

[ -x "$BIN" ] || { echo "webserv introuvable, lance 'make' d'abord"; exit 2; }
command -v python3 >/dev/null || { echo "python3 requis"; exit 2; }
command -v curl    >/dev/null || { echo "curl requis"; exit 2; }
if [ "$VG" = "1" ]; then
	command -v valgrind >/dev/null || { echo "valgrind requis pour --valgrind"; exit 2; }
fi

PY="$(command -v python3)"
PASS=0
FAIL=0

ok()   { PASS=$((PASS+1)); printf '  \033[32mOK\033[0m   %s\n' "$1"; }
ko()   { FAIL=$((FAIL+1)); printf '  \033[31mKO\033[0m   %s\n' "$1"; }
info() { printf '       %s\n' "$1"; }

# --- Racine jetable : les fixtures ne polluent pas www/ du repo -------------
WWW="$(mktemp -d /tmp/cgilife_XXXXXX)"
mkdir -p "$WWW/cgi-bin"

cat > "$WWW/cgi-bin/hello.py" <<'EOF'
#!/usr/bin/env python3
print("Content-Type: text/plain\r\n\r\nHello from CGI")
EOF

# Ne meurt jamais : sert a declencher CGI_TIMEOUT.
cat > "$WWW/cgi-bin/loop.py" <<'EOF'
#!/usr/bin/env python3
while True: pass
EOF

# Ferme stdout tout de suite mais reste vivant : le serveur voit EOF alors que
# waitpid(WNOHANG) rend encore 0. C'est le cas qui declenche les bugs 1 et 2.
cat > "$WWW/cgi-bin/lazy.py" <<'EOF'
#!/usr/bin/env python3
import sys, time, os
sys.stdout.write("Content-Type: text/plain\r\n\r\nPARTIAL")
sys.stdout.flush()
os.close(1)
time.sleep(30)
EOF

CONF="$(mktemp /tmp/cgilife_XXXXXX.conf)"
LOG="$(mktemp /tmp/cgilife_XXXXXX.log)"
VGLOG="$(mktemp /tmp/cgilife_XXXXXX.vg)"

cat > "$CONF" <<EOF
server
{
	listen					$PORT;
	server_name				webserv;
	client_max_body_size	10M;

	location /
	{
		allow_methods		GET;
		root				$WWW;
		index				index.html;
	}

	location /cgi-bin
	{
		allow_methods		GET POST;
		root				$WWW;
		cgi_ext				.py;
		cgi_pass			$PY;
	}
}
EOF

cleanup() {
	if [ "$KEEP" = "0" ]; then
		if [ -n "${SRV_PID:-}" ] && kill -0 "$SRV_PID" 2>/dev/null; then
			kill "$SRV_PID" 2>/dev/null
			wait "$SRV_PID" 2>/dev/null
		fi
		rm -rf "$WWW"
		rm -f "$CONF" "$LOG" "$VGLOG"
	fi
}
trap cleanup EXIT

if [ "$VG" = "1" ]; then
	valgrind --leak-check=full --track-fds=yes --log-file="$VGLOG" \
		"$BIN" "$CONF" > "$LOG" 2>&1 &
else
	"$BIN" "$CONF" > "$LOG" 2>&1 &
fi
SRV_PID=$!
python3 -c 'import time; time.sleep(2)'

if ! kill -0 "$SRV_PID" 2>/dev/null; then
	echo "le serveur n'a pas demarre sur le port $PORT :"
	cat "$LOG"
	echo "(relance avec --port <autre> si le port est occupe)"
	exit 2
fi

# Sous valgrind le PID a surveiller est celui du process trace, pas du wrapper.
SRV_WATCH="$SRV_PID"
if [ "$VG" = "1" ]; then
	W="$(pgrep -P "$SRV_PID" -f webserv | head -1)"
	[ -n "$W" ] && SRV_WATCH="$W"
fi

URL="http://127.0.0.1:$PORT/cgi-bin"
kids() { ps --ppid "$SRV_WATCH" -o pid= 2>/dev/null | wc -l; }
fds()  { ls "/proc/$SRV_WATCH/fd" 2>/dev/null | wc -l; }

echo "webserv PID=$SRV_WATCH  port=$PORT  root=$WWW"
[ "$VG" = "1" ] && echo "(sous valgrind : les temps sont fausses, le test 1 est allege)"
echo

FD_START="$(fds)"

# --- 1. Course a la recolte ------------------------------------------------
# Le coeur du ticket. Une requete pendue = curl rend 000. Une requete "lente"
# (>200ms sur un script qui repond en 10ms) signale un poll() qui a dormi
# jusqu'a son timeout au lieu de repasser sur SweepPendingReap.
echo "[ 1. recolte sous charge - $( [ "$VG" = "1" ] && echo 25 || echo 100 ) requetes ]"
N=100
[ "$VG" = "1" ] && N=25
nok=0; nhang=0; nslow=0
for i in $(seq 1 $N); do
	t="$(curl -s -m 5 -o /dev/null -w '%{http_code} %{time_total}' "$URL/hello.py")"
	code="${t%% *}"; dur="${t##* }"
	if [ "$code" = "200" ]; then
		nok=$((nok+1))
		awk "BEGIN{exit !($dur > 0.2)}" && nslow=$((nslow+1))
	else
		nhang=$((nhang+1))
	fi
done
info "200=$nok  pendues=$nhang  lentes(>200ms)=$nslow"
[ "$nhang" = "0" ] && ok "aucune requete sans reponse" \
                   || ko "$nhang/$N requetes sans reponse (bug 1 : return sec sur Reap)"
[ "$nslow" = "0" ] && ok "aucune latence anormale" \
                   || ko "$nslow/$N requetes >200ms (ComputeTimeout ignore _PendingReap)"

# --- 2. Zero zombie --------------------------------------------------------
echo
echo "[ 2. zombies apres la charge ]"
python3 -c 'import time; time.sleep(2)'
K="$(kids)"
info "enfants restants = $K"
[ "$K" = "0" ] && ok "aucun processus fils residuel" \
               || ko "$K processus fils non recoltes"

# --- 3. Timeout CGI --------------------------------------------------------
# CGI_TIMEOUT vaut 10s (includes/Default.hpp) : on attend un 504 entre 9 et 14s.
echo
echo "[ 3. timeout CGI sur un script infini ]"
t="$(curl -s -m 25 -o /dev/null -w '%{http_code} %{time_total}' "$URL/loop.py")"
code="${t%% *}"; dur="${t##* }"
info "code=$code  temps=${dur}s"
[ "$code" = "504" ] && ok "504 Gateway Timeout renvoye" \
                    || ko "attendu 504, recu $code"
awk "BEGIN{exit !($dur > 9 && $dur < 14)}" \
	&& ok "expire dans la fenetre CGI_TIMEOUT (9-14s)" \
	|| ko "temps hors fenetre : ${dur}s pour CGI_TIMEOUT=10"
python3 -c 'import time; time.sleep(1)'
K="$(kids)"
[ "$K" = "0" ] && ok "le script infini a bien ete tue" \
               || ko "$K fils survivent au timeout (SIGKILL absent)"

# --- 4. Pas de double reponse ----------------------------------------------
# curl ne peut PAS voir ce bug : il lit Content-Length octets et s'arrete.
# Il faut lire le socket jusqu'a EOF et compter les lignes de statut.
echo
echo "[ 4. une seule reponse HTTP par connexion ]"
RES="$(PORT="$PORT" python3 - <<'PYEOF'
import os, socket
port = int(os.environ["PORT"])
s = socket.create_connection(("127.0.0.1", port)); s.settimeout(20)
s.sendall(b"GET /cgi-bin/lazy.py HTTP/1.1\r\nHost: webserv\r\n\r\n")
buf = b""
eof = 1
try:
	while True:
		d = s.recv(4096)
		if not d:
			break
		buf += d
except Exception:
	eof = 0        # pas de EOF alors que la reponse annonce Connection: close
print("%d %d %d" % (len(buf), buf.count(b"HTTP/1."), eof))
PYEOF
)"
set -- $RES
NBYTES="$1"; NSTATUS="$2"; NEOF="$3"
info "octets=$NBYTES  lignes de statut=$NSTATUS  eof=$NEOF"
[ "$NSTATUS" = "1" ] && ok "une seule reponse sur le fil" \
                     || ko "$NSTATUS reponses HTTP empilees (bug 2 : 504 puis 200)"
[ "$NEOF" = "1" ] && ok "connexion fermee comme annonce" \
                  || ko "Connection: close annonce mais socket laissee ouverte"

# --- 5. Deconnexion du client pendant le CGI -------------------------------
# Le client part avant la fin : CloseConnection doit tuer l'enfant tout de
# suite, sans attendre CGI_TIMEOUT.
echo
echo "[ 5. client qui raccroche pendant le CGI ]"
for i in 1 2 3; do curl -s -m 2 -o /dev/null "$URL/loop.py"; done
python3 -c 'import time; time.sleep(1)'
K="$(kids)"
info "enfants 1s apres la deconnexion = $K"
[ "$K" = "0" ] && ok "CGI tue a la fermeture de la connexion" \
               || ko "$K fils orphelins (CloseConnection ne tue pas le CGI)"

# --- 6. Fds stables --------------------------------------------------------
echo
echo "[ 6. descripteurs ]"
FD_END="$(fds)"
info "fds debut=$FD_START  fin=$FD_END"
[ "$FD_END" -le "$FD_START" ] && ok "aucun fd ne fuit" \
                              || ko "$((FD_END - FD_START)) fd(s) en fuite"

# --- 7. SIGINT pendant un CGI ----------------------------------------------
echo
echo "[ 7. SIGINT avec un CGI en cours ]"
curl -s -m 25 -o /dev/null "$URL/loop.py" &
CURL_PID=$!
python3 -c 'import time; time.sleep(2)'
kill -INT "$SRV_PID" 2>/dev/null
python3 -c 'import time; time.sleep(3)'
if kill -0 "$SRV_WATCH" 2>/dev/null; then
	ko "webserv ne sort pas sur SIGINT"
	kill -9 "$SRV_WATCH" 2>/dev/null
else
	ok "sortie propre sur SIGINT"
fi
kill "$CURL_PID" 2>/dev/null; wait "$CURL_PID" 2>/dev/null
ORPH="$(ps -eo args | grep -F "$WWW/cgi-bin" | grep -v grep | wc -l)"
[ "$ORPH" = "0" ] && ok "aucun CGI orphelin apres l'arret" \
                  || ko "$ORPH CGI orphelin(s) reparente(s) a init"
SRV_PID=""

# --- 8. Valgrind -----------------------------------------------------------
if [ "$VG" = "1" ]; then
	echo
	echo "[ 8. valgrind ]"
	python3 -c 'import time; time.sleep(3)'
	ERRS="$(grep -oP 'ERROR SUMMARY: \K[0-9]+' "$VGLOG" | tail -1)"
	FDLINE="$(grep 'FILE DESCRIPTORS' "$VGLOG" | tail -1)"
	LOST="$(grep -cE '(definitely|indirectly) lost: [1-9]' "$VGLOG")"
	info "${FDLINE:-pas de ligne FILE DESCRIPTORS}"
	[ "${ERRS:-1}" = "0" ] && ok "0 erreur memoire" \
	                       || { ko "${ERRS:-?} erreur(s) memoire (bug 3 : use-after-free)"
	                            grep -A4 'Invalid \(read\|write\)' "$VGLOG" | head -12; }
	[ "$LOST" = "0" ] && ok "aucune fuite memoire" \
	                  || ko "fuites memoire signalees"
fi

# --- Bilan -----------------------------------------------------------------
echo
echo "-------------------------------------------"
if [ "$FAIL" = "0" ]; then
	printf '\033[32m%d/%d controles passes -- D-05 conforme.\033[0m\n' "$PASS" "$PASS"
	exit 0
fi
printf '\033[31m%d echec(s) sur %d controles.\033[0m\n' "$FAIL" "$((PASS+FAIL))"
[ -s "$LOG" ] && { echo "--- log serveur ---"; tail -20 "$LOG"; }
exit 1
