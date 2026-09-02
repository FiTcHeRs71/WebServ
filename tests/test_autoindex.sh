#!/usr/bin/env bash
# Teste l'autoindex (C-07) : listing HTML genere quand "autoindex on" et
# qu'aucun fichier d'index n'a ete trouve dans le dossier.
#
# Le script cree son PROPRE bac a sable dans /tmp avec des noms de fichiers
# pieges (balise HTML, esperluette, guillemet, espace, accent, fichier cache)
# et sa propre conf : YoupiBanane/ n'est jamais modifie, le tester officiel 42
# lit ce dossier et n'a pas a voir nos fichiers de test.
#
# Il demarre sa propre instance de webserv et retient son PID via $! : pas de
# `pidof webserv`, qui renverrait plusieurs PID si une autre instance tourne.
#
# Usage :
#   ./tests/test_autoindex.sh                # tout
#   ./tests/test_autoindex.sh --port 9500    # si 8097 est pris
#   ./tests/test_autoindex.sh --valgrind     # ajoute un check memoire
#   ./tests/test_autoindex.sh --keep         # garde le bac a sable et la conf

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/webserv"
PORT=8097
KEEP=0
USE_VALGRIND=0

while [ $# -gt 0 ]; do
	case "$1" in
		--port) PORT="$2"; shift 2 ;;
		--keep) KEEP=1; shift ;;
		--valgrind) USE_VALGRIND=1; shift ;;
		*) echo "usage: $0 [--port N] [--keep] [--valgrind]"; exit 2 ;;
	esac
done

[ -x "$BIN" ] || { echo "webserv introuvable, lance 'make' d'abord"; exit 2; }
command -v curl >/dev/null || { echo "curl requis"; exit 2; }

pass=0
fail=0

# ---------------------------------------------------------------- fixtures

SANDBOX="$(mktemp -d /tmp/autoindex_XXXXXX)"
CONF="$(mktemp /tmp/autoindex_XXXXXX.conf)"
LOG="$(mktemp /tmp/autoindex_XXXXXX.log)"
VG_LOG="$(mktemp /tmp/autoindex_vg_XXXXXX.log)"

# Dossiers : tries en tete du listing, ordre ASCII (Y=89 < n=110 < w=119)
mkdir -p "$SANDBOX/Yeah" "$SANDBOX/nop" "$SANDBOX/withindex" "$SANDBOX/locked"

# Fichiers : ordre ASCII attendu -> '.'(46) < '<'(60) < 'a' < 'm' < 't' < 'y'
touch "$SANDBOX/.hidden"
touch "$SANDBOX/<img src=x onerror=alert(1)>.txt"
touch "$SANDBOX/a&b\"c.txt"
touch "$SANDBOX/mon fichier é.txt"
printf 'abc' > "$SANDBOX/test.txt"
printf 'youpi' > "$SANDBOX/youpi.bla"

printf '<h1>INDEX SERVI</h1>\n' > "$SANDBOX/withindex/index.html"

# Dossier illisible : opendir() doit echouer -> 403 (inoperant en root)
chmod 000 "$SANDBOX/locked"

cat > "$CONF" <<EOF
server {
	listen			$PORT;
	server_name		localhost;

	location / {
		allow_methods	GET;
		root			$SANDBOX;
		autoindex		on;
	}

	location /refuse/ {
		allow_methods	GET;
		root			$SANDBOX;
		autoindex		off;
	}

	location /withindex/ {
		allow_methods	GET;
		root			$SANDBOX/withindex;
		index			index.html;
		autoindex		on;
	}
}
EOF

cleanup() {
	if [ -n "${SRV_PID:-}" ] && kill -0 "$SRV_PID" 2>/dev/null; then
		kill "$SRV_PID" 2>/dev/null
		wait "$SRV_PID" 2>/dev/null
	fi
	chmod 755 "$SANDBOX/locked" 2>/dev/null
	if [ "$KEEP" = "0" ]; then
		rm -rf "$SANDBOX"
		rm -f "$CONF" "$LOG" "$VG_LOG"
	else
		echo "conserve : $SANDBOX  $CONF  $LOG"
	fi
}
trap cleanup EXIT

# ---------------------------------------------------------------- helpers

ok() {
	printf 'PASS  %s\n' "$1"
	pass=$((pass + 1))
}

ko() {
	printf 'FAIL  %s\n' "$1"
	shift
	while [ $# -gt 0 ]; do printf '        %s\n' "$1"; shift; done
	fail=$((fail + 1))
}

body() { curl -s -m 3 "http://127.0.0.1:$PORT$1"; }
headers() { curl -s -m 3 -D - -o /dev/null "http://127.0.0.1:$PORT$1"; }
code() { curl -s -m 3 -o /dev/null -w '%{http_code}' "http://127.0.0.1:$PORT$1"; }

# curl -I enverrait un HEAD, que le serveur rend en 501 : on reste en GET.
check_code() {
	local uri="$1" want="$2" desc="$3" got
	got="$(code "$uri")"
	[ "$got" = "$want" ] && ok "$desc" || ko "$desc" "attendu $want, recu $got"
}

check_has() {
	local hay="$1" needle="$2" desc="$3"
	case "$hay" in
		*"$needle"*) ok "$desc" ;;
		*) ko "$desc" "chaine absente : $needle" ;;
	esac
}

check_hasnt() {
	local hay="$1" needle="$2" desc="$3"
	case "$hay" in
		*"$needle"*) ko "$desc" "chaine presente alors qu'elle ne devrait pas : $needle" ;;
		*) ok "$desc" ;;
	esac
}

# Extrait la suite des href dans l'ordre du document
hrefs() {
	printf '%s' "$1" | grep -o 'href="[^"]*"' | sed 's/^href="//; s/"$//'
}

# ---------------------------------------------------------------- demarrage

if [ "$USE_VALGRIND" = "1" ]; then
	command -v valgrind >/dev/null || { echo "valgrind requis avec --valgrind"; exit 2; }
	valgrind --leak-check=full --track-fds=yes --quiet \
		--log-file="$VG_LOG" "$BIN" "$CONF" > "$LOG" 2>&1 &
else
	"$BIN" "$CONF" > "$LOG" 2>&1 &
fi
SRV_PID=$!
sleep 1

if ! kill -0 "$SRV_PID" 2>/dev/null; then
	echo "le serveur n'a pas demarre sur le port $PORT :"
	cat "$LOG"
	echo "(relance avec --port <autre> si le port est occupe)"
	exit 2
fi

echo "=== Autoindex (C-07) sur $SANDBOX, port $PORT ==="

ROOT_BODY="$(body /)"
SUB_BODY="$(body /Yeah/)"

# --- 1. la page existe et s'annonce correctement -------------------------

check_code / 200 "GET / -> 200 (autoindex on, aucun index)"
check_has "$(headers /)" "text/html" "Content-Type: text/html"
check_has "$(headers /)" "charset=utf-8" "charset=utf-8 (noms accentues lisibles)"
check_has "$ROOT_BODY" "<!DOCTYPE html>" "page HTML complete (doctype)"
check_has "$ROOT_BODY" "<title>Index of /</title>" "titre = Index of /"
check_has "$ROOT_BODY" "</html>" "HTML referme"

# --- 2. ordre : dossiers d'abord, puis alphabetique ----------------------

WANT_ORDER='Yeah/
locked/
nop/
withindex/
.hidden
%3Cimg%20src%3Dx%20onerror%3Dalert%281%29%3E.txt
a%26b%22c.txt
mon%20fichier%20%C3%A9.txt
test.txt
youpi.bla'
GOT_ORDER="$(hrefs "$ROOT_BODY")"
if [ "$GOT_ORDER" = "$WANT_ORDER" ]; then
	ok "ordre : dossiers d'abord puis alphabetique"
else
	ko "ordre : dossiers d'abord puis alphabetique" \
		"attendu :" "$WANT_ORDER" "recu :" "$GOT_ORDER"
fi

# --- 3. entrees speciales ------------------------------------------------

check_hasnt "$ROOT_BODY" 'href="./"' "'.' jamais affiche"
check_hasnt "$ROOT_BODY" 'href="../"' "pas de lien parent a la racine"
check_has "$SUB_BODY" 'href="../"' "lien parent present dans un sous-dossier"

# --- 4. echappement HTML (XSS stockee) -----------------------------------

check_hasnt "$ROOT_BODY" '<img src=x' "aucune balise <img> injectee (XSS)"
check_has "$ROOT_BODY" '&lt;img src=x onerror=alert(1)&gt;.txt' "nom piege echappe en entites"
check_has "$ROOT_BODY" 'a&amp;b&quot;c.txt' "& et \" echappes"
check_hasnt "$ROOT_BODY" '>a&b"c.txt<' "pas de & ni de \" bruts dans le texte"

# --- 5. encodage des liens ----------------------------------------------

check_has "$ROOT_BODY" 'href="mon%20fichier%20%C3%A9.txt"' "href URL-encode (espace + UTF-8)"
check_has "$ROOT_BODY" 'mon fichier é.txt</a>' "texte affiche reste lisible"
check_code '/mon%20fichier%20%C3%A9.txt' 200 "le lien encode est suivable (200)"

# --- 6. navigation : pas de 301 sur les dossiers -------------------------

check_has "$ROOT_BODY" 'href="Yeah/"' "href de dossier termine par '/'"
check_code /Yeah/ 200 "descente dans un sous-dossier -> 200 direct"

# --- 7. colonnes taille --------------------------------------------------

check_has "$ROOT_BODY" '>Yeah/</a></td><td>-</td>' "taille d'un dossier affichee '-'"
check_has "$ROOT_BODY" '>youpi.bla</a></td><td>5</td>' "taille d'un fichier en octets"

# --- 8. arbre de decision ------------------------------------------------

check_code /refuse/ 403 "autoindex off -> 403 (pas 404, pas de listing)"
check_hasnt "$(body /refuse/)" "<table>" "autoindex off -> aucun listing dans le body"
check_has "$(body /withindex/)" "INDEX SERVI" "index prioritaire sur l'autoindex"
check_code /nexistepas/ 404 "dossier inexistant -> 404"

if [ "$(id -u)" = "0" ]; then
	echo "SKIP  opendir() refuse -> 403 (root ignore les permissions)"
else
	check_code /locked/ 403 "opendir() refuse -> 403"
fi

# --- 9. memoire et fd ----------------------------------------------------

if [ "$USE_VALGRIND" = "1" ]; then
	kill -TERM "$SRV_PID" 2>/dev/null
	wait "$SRV_PID" 2>/dev/null
	SRV_PID=""
	if grep -q "definitely lost: [1-9]" "$VG_LOG" \
		|| grep -q "indirectly lost: [1-9]" "$VG_LOG"; then
		ko "valgrind : aucune fuite memoire" "voir $VG_LOG"
		KEEP=1
	else
		ok "valgrind : aucune fuite memoire"
	fi
	# Valgrind ne liste que les fd non standards encore ouverts a la sortie.
	# Le sien ($VG_LOG, herite) en fait partie : on ne compte que les autres.
	leaked_fds="$(grep 'Open file descriptor' "$VG_LOG" | grep -vc "$VG_LOG")"
	if [ "$leaked_fds" -eq 0 ]; then
		ok "valgrind : aucun fd fuite (closedir)"
	else
		ko "valgrind : aucun fd fuite (closedir)" \
			"$leaked_fds fd encore ouvert(s) hors std" "voir $VG_LOG"
		KEEP=1
	fi
fi

echo
echo "=== Resume : $pass reussi(s), $fail echoue(s) ==="
[ "$fail" -eq 0 ]
