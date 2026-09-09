#!/usr/bin/env bash
# Verrouille la Definition of Done du ticket C-11 (methode DELETE).
#
# Le test 1 passe EN PREMIER, et c'est volontaire : un DELETE qui s'echappe de
# sa location efface de vrais fichiers. Tant qu'il n'est pas vert, aucun autre
# test ne doit tourner.
#
# Deux pieges que ce script contourne, et qui font perdre des heures :
#
#   1. curl NORMALISE l'URL avant de l'envoyer : "/upload/../../Makefile" part
#      sur le reseau sous la forme "/Makefile". On teste alors curl, pas
#      webserv -- et on obtient un 405 rassurant qui ne prouve rien. L'option
#      --path-as-is desactive cette reecriture. Elle est obligatoire ici.
#
#   2. Sur une location GET-only, un DELETE d'un fichier inexistant rend 405 et
#      non 404 : allow_methods est verifie avant tout acces disque
#      (Router.cpp:370). Ce n'est pas un bug, c'est le bon ordre -- on ne
#      divulgue pas l'existence d'une ressource sur une methode interdite.
#
# Decisions du ticket verifiees ici : succes = 204 sans corps ; dossier = 403.
#
# Usage :
#   ./tests/test_delete.sh              # tout
#   ./tests/test_delete.sh --port 9500  # si 8098 est pris
#   ./tests/test_delete.sh --keep       # laisse le serveur et le bac a sable

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/webserv"
PORT=8098
KEEP=0

while [ $# -gt 0 ]; do
	case "$1" in
		--port) PORT="$2"; shift 2 ;;
		--keep) KEEP=1; shift ;;
		*) echo "usage: $0 [--port N] [--keep]"; exit 2 ;;
	esac
done

[ -x "$BIN" ] || { echo "webserv introuvable, lance 'make' d'abord"; exit 2; }
command -v curl    >/dev/null || { echo "curl requis"; exit 2; }
command -v python3 >/dev/null || { echo "python3 requis"; exit 2; }
curl --path-as-is -s -o /dev/null file:///dev/null 2>/dev/null
[ $? -eq 2 ] && { echo "curl trop ancien : --path-as-is indisponible"; exit 2; }

PASS=0
FAIL=0
ok()   { PASS=$((PASS+1)); printf '  \033[32mOK\033[0m   %s\n' "$1"; }
ko()   { FAIL=$((FAIL+1)); printf '  \033[31mKO\033[0m   %s\n' "$1"; }
info() { printf '       %s\n' "$1"; }
nap()  { python3 -c "import time; time.sleep($1)"; }

# --- Bac a sable : rien ne pointe vers le depot ----------------------------
WWW="$(mktemp -d /tmp/deltest_XXXXXX)"
mkdir -p "$WWW/upload/sousdossier"
echo "<h1>index</h1>"  > "$WWW/index.html"
echo "hors de la zone" > "$WWW/hors_zone.txt"          # 1 cran au-dessus du root
CANARI="$(mktemp /tmp/deltest_canari_XXXXXX)"          # 2 crans au-dessus
echo "canari" > "$CANARI"

CONF="$(mktemp /tmp/deltest_XXXXXX.conf)"
LOG="$(mktemp /tmp/deltest_XXXXXX.log)"

# root de /upload = le dossier d'upload lui-meme. Le sujet impose la semantique
# `alias` (p.307 : /kapouet rooted to /tmp/www -> /tmp/www/pouic), donc
# "/upload/x" resout vers "$WWW/upload/x" et isInsideRoot confine la
# suppression a ce seul dossier.
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

	location /upload
	{
		allow_methods		GET POST DELETE;
		root				$WWW/upload;
		autoindex			on;
		upload_store		$WWW/upload;
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
		rm -f "$CONF" "$LOG" "$CANARI"
	else
		echo "--keep : serveur PID ${SRV_PID:-?}, bac a sable $WWW"
	fi
}
trap cleanup EXIT

"$BIN" "$CONF" > "$LOG" 2>&1 &
SRV_PID=$!
nap 1.5

if ! kill -0 "$SRV_PID" 2>/dev/null; then
	echo "le serveur n'a pas demarre sur le port $PORT :"
	cat "$LOG"
	echo "(relance avec --port <autre> si le port est occupe)"
	exit 2
fi

U="http://127.0.0.1:$PORT"
# Envoie la requete telle quelle, sans normalisation cliente.
raw()  { curl -s --path-as-is -o /dev/null -w '%{http_code}' -X DELETE "$U$1"; }
code() { curl -s -o /dev/null -w '%{http_code}' -X "$1" "$U$2"; }

echo "webserv PID=$SRV_PID  port=$PORT  bac a sable=$WWW"
echo

# --- 1. Path traversal : LE test du ticket ---------------------------------
echo "[ 1. path traversal - aucun DELETE ne doit sortir de la location ]"
TRAV="/upload/../hors_zone.txt
/upload/../../$(basename "$CANARI")
/upload/../index.html
/upload/./../../$(basename "$CANARI")
/upload/..%2f..%2f$(basename "$CANARI")
/upload/../../../../../../../../etc/passwd"

nbad=0
while IFS= read -r u; do
	c="$(raw "$u")"
	if [ "$c" = "403" ]; then
		info "403  $u"
	else
		nbad=$((nbad+1))
		info "$c  $u   <-- ATTENDU 403"
	fi
done <<< "$TRAV"
[ "$nbad" = "0" ] && ok "toutes les tentatives d'evasion rendent 403" \
                  || ko "$nbad tentative(s) non bloquee(s) par isInsideRoot()"

nlost=0
for f in "$WWW/hors_zone.txt" "$WWW/index.html" "$CANARI" /etc/passwd; do
	[ -e "$f" ] || { nlost=$((nlost+1)); info "DETRUIT : $f"; }
done
[ "$nlost" = "0" ] && ok "tous les fichiers temoins sont intacts" \
                   || ko "$nlost fichier(s) temoin(s) supprime(s) hors de la location"

if [ "$nbad" != "0" ] || [ "$nlost" != "0" ]; then
	echo
	printf '\033[31mARRET : le path traversal n_est pas bloque, on ne va pas plus loin.\033[0m\n'
	exit 1
fi

# --- 2. Suppression nominale : 204 sans corps ------------------------------
echo
echo "[ 2. suppression nominale ]"
echo "photo" > "$WWW/upload/photo.png"
[ "$(curl -s "$U/upload/" | grep -c 'photo.png')" -ge 1 ] \
	&& ok "l'autoindex liste photo.png avant suppression" \
	|| ko "photo.png absent du listing (C-07 casse ?)"

HDR="$(curl -s -i -X DELETE "$U/upload/photo.png")"
echo "$HDR" | head -1 | grep -q '204 No Content' \
	&& ok "DELETE rend 204 No Content" \
	|| { ko "statut inattendu : $(echo "$HDR" | head -1 | tr -d '\r')"; }

# Un 204 annonce "pas de contenu" : ni corps, ni Content-Length. Un
# Content-Length parasite fait attendre des octets qui n'arriveront jamais.
echo "$HDR" | grep -qi '^Content-Length:' \
	&& ko "le 204 porte un Content-Length" \
	|| ok "le 204 ne porte pas de Content-Length"
[ "$(echo "$HDR" | sed -n '/^\r*$/,$p' | tr -d '\r\n')" = "" ] \
	&& ok "le 204 n'a pas de corps" \
	|| ko "le 204 transporte un corps"

[ -e "$WWW/upload/photo.png" ] \
	&& ko "le fichier est toujours sur le disque" \
	|| ok "le fichier a bien ete supprime du disque"
[ "$(curl -s "$U/upload/" | grep -c 'photo.png')" = "0" ] \
	&& ok "photo.png a disparu de l'autoindex" \
	|| ko "photo.png encore liste apres suppression"

# --- 3. Idempotence --------------------------------------------------------
echo
echo "[ 3. idempotence ]"
c="$(code DELETE /upload/photo.png)"
[ "$c" = "404" ] && ok "un second DELETE rend 404" \
                 || ko "second DELETE : $c (404 attendu)"

# --- 4. Dossiers : jamais de suppression recursive -------------------------
echo
echo "[ 4. dossiers ]"
for u in /upload/ /upload /upload/sousdossier; do
	c="$(code DELETE "$u")"
	[ "$c" = "403" ] && ok "DELETE $u rend 403" \
	                 || ko "DELETE $u rend $c (403 attendu)"
done
[ -d "$WWW/upload/sousdossier" ] && ok "le sous-dossier est intact" \
                                || ko "le sous-dossier a ete supprime"

# --- 5. allow_methods (C-09) -----------------------------------------------
echo
echo "[ 5. methode non autorisee ]"
H="$(curl -s -i -X DELETE "$U/index.html")"
echo "$H" | head -1 | grep -q '405' \
	&& ok "DELETE sur une location GET-only rend 405" \
	|| ko "statut inattendu : $(echo "$H" | head -1 | tr -d '\r')"
echo "$H" | grep -qi '^Allow:.*GET' \
	&& ok "le 405 porte un en-tete Allow" \
	|| ko "le 405 ne porte pas de Allow (obligatoire, RFC 7231)"
[ -e "$WWW/index.html" ] && ok "index.html n'a pas ete supprime" \
                         || ko "index.html supprime malgre le 405"

# Le check allow_methods precede le stat() : sur une location GET-only, un
# fichier inexistant rend 405, pas 404. On ne renseigne pas le client sur
# l'existence d'une ressource qu'il n'a pas le droit de supprimer.
c="$(code DELETE /nexiste_pas.txt)"
[ "$c" = "405" ] && ok "405 rendu avant tout acces disque" \
                 || ko "DELETE d'un fichier absent en GET-only rend $c (405 attendu)"

# --- 6. cycle complet C-10 -> C-11 -----------------------------------------
# POST ecrit dans `upload_store`, DELETE resout l'URI par build_path, donc par
# le `root`. Si les deux ne designent pas le meme dossier, le fichier uploade
# n'est adressable par aucune URI : POST rend 201, puis le listing est vide et
# le DELETE rend 404. Le cycle du ticket est casse sans qu'aucun test unitaire
# ne rougisse -- c'est exactement ce qui est arrive avec conf/default.conf.
# Ce bloc verrouille l'invariant : upload_store doit etre sous le root.
echo
echo "[ 6. cycle POST -> listing -> DELETE ]"
c="$(curl -s --path-as-is -o /dev/null -w '%{http_code}' \
	-X POST --data-binary 'cycle' "$U/upload/cycle.txt")"
[ "$c" = "201" ] && ok "POST rend 201 Created" \
                 || ko "POST rend $c (201 attendu)"

[ "$(curl -s "$U/upload/" | grep -c 'cycle.txt')" -ge 1 ] \
	&& ok "le fichier uploade apparait dans l'autoindex" \
	|| ko "cycle.txt absent du listing (upload_store hors du root ?)"

c="$(code GET /upload/cycle.txt)"
[ "$c" = "200" ] && ok "GET relit le fichier uploade" \
                 || ko "GET rend $c (200 attendu)"

c="$(raw /upload/cycle.txt)"
[ "$c" = "204" ] && ok "DELETE supprime le fichier uploade" \
                 || ko "DELETE rend $c (404 = POST et DELETE ne visent pas le meme dossier)"

# find sur tout le bac a sable, pas seulement sur $WWW/upload : si POST a
# ecrit hors du root, le fichier survit ailleurs et le DELETE ne l'a jamais vu.
[ -z "$(find "$WWW" -name cycle.txt 2>/dev/null)" ] \
	&& ok "le cycle POST -> DELETE ne laisse rien derriere lui" \
	|| ko "cycle.txt survit sur le disque : $(find "$WWW" -name cycle.txt)"

# --- Bilan -----------------------------------------------------------------
echo
echo "-------------------------------------------"
if [ "$FAIL" = "0" ]; then
	printf '\033[32m%d/%d controles passes -- C-11 conforme.\033[0m\n' "$PASS" "$PASS"
	exit 0
fi
printf '\033[31m%d echec(s) sur %d controles.\033[0m\n' "$FAIL" "$((PASS+FAIL))"
[ -s "$LOG" ] && { echo "--- log serveur ---"; tail -20 "$LOG"; }
exit 1
