#!/usr/bin/env bash
# Tests D-04 : parsing de la sortie CGI.
# Usage: ./tests/test_cgi_output.sh [--port PORT] [--keep]

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/webserv"
PORT=8100
KEEP=0
PASS=0
FAIL=0

while [ $# -gt 0 ]; do
	case "$1" in
		--port) PORT="$2"; shift 2 ;;
		--keep) KEEP=1; shift ;;
		*) echo "usage: $0 [--port PORT] [--keep]"; exit 2 ;;
	esac
done

[ -x "$BIN" ] || { echo "webserv introuvable : lance make d'abord"; exit 2; }
command -v curl >/dev/null || { echo "curl requis"; exit 2; }
command -v python3 >/dev/null || { echo "python3 requis"; exit 2; }

WWW="$(mktemp -d /tmp/cgi_output_XXXXXX)"
CONF="$(mktemp /tmp/cgi_output_XXXXXX.conf)"
LOG="$(mktemp /tmp/cgi_output_XXXXXX.log)"
SRV_PID=""

make_cgi() {
	local name="$1"
	local content="$2"
	printf '%s\n' "$content" > "$WWW/cgi-bin/$name.py"
}

mkdir -p "$WWW/cgi-bin"
make_cgi normal 'import sys; sys.stdout.write("Content-Type: text/plain\r\n\r\nhello")'
make_cgi empty_body 'import sys; sys.stdout.write("Content-Type: text/plain\r\n\r\n")'
make_cgi status 'print("Status: 404 Not Found\r\nContent-Type: text/plain\r\n\r\nnot found")'
make_cgi redirect 'print("Location: /index.html\r\n\r\n")'
make_cgi bad_status 'print("Status: abc\r\nContent-Type: text/plain\r\n\r\ninvalid")'
make_cgi out_of_range 'print("Status: 600 Invalid\r\n\r\ninvalid")'
make_cgi no_headers 'print("not CGI headers")'
make_cgi no_output 'pass'

cat > "$CONF" <<EOF
server
{
	listen				$PORT;
	server_name			webserv;

	location /
	{
		allow_methods		GET;
		root			$WWW;
	}

	location /cgi-bin
	{
		allow_methods		GET;
		root			$WWW;
		cgi_ext			.py;
		cgi_pass		$(command -v python3);
	}
}
EOF

cleanup() {
	if [ -n "$SRV_PID" ] && kill -0 "$SRV_PID" 2>/dev/null; then
		kill "$SRV_PID" 2>/dev/null
		wait "$SRV_PID" 2>/dev/null
	fi
	if [ "$KEEP" = "0" ]; then
		rm -rf "$WWW" "$CONF" "$LOG"
	else
		echo "fixtures conserves : $WWW"
		echo "configuration conservee : $CONF"
		echo "log conserve : $LOG"
	fi
}
trap cleanup EXIT

ok() {
	PASS=$((PASS + 1))
	printf 'PASS  %s\n' "$1"
}

ko() {
	FAIL=$((FAIL + 1))
	printf 'FAIL  %s\n' "$1"
	printf '      obtenu : %s\n' "$2"
}

check_code() {
	local path="$1"
	local expected="$2"
	local label="$3"
	local got
	got="$(curl -sS -m 3 -o /tmp/cgi_output_body -w '%{http_code}' "http://127.0.0.1:$PORT$path")"
	if [ "$got" = "$expected" ]; then
		ok "$label"
	else
		ko "$label" "$got, attendu $expected"
	fi
}

check_header() {
	local path="$1"
	local expected="$2"
	local label="$3"
	local headers
	headers="$(curl -sS -m 3 -D - -o /dev/null "http://127.0.0.1:$PORT$path")"
	case "$headers" in
		*"$expected"*) ok "$label" ;;
		*) ko "$label" "header absent : $expected" ;;
	esac
}

check_body_has() {
	local path="$1"
	local expected="$2"
	local label="$3"
	local body
	body="$(curl -sS -m 3 "http://127.0.0.1:$PORT$path")"
	case "$body" in
		*"$expected"*) ok "$label" ;;
		*) ko "$label" "body absent : $expected" ;;
	esac
}

"$BIN" "$CONF" > "$LOG" 2>&1 &
SRV_PID=$!

ready=0
for _ in 1 2 3 4 5 6 7 8 9 10; do
	if curl -sS -m 1 "http://127.0.0.1:$PORT/" >/dev/null 2>&1; then
		ready=1
		break
	fi
	done
if [ "$ready" -eq 0 ]; then
	echo "serveur non demarre :"
	cat "$LOG"
	exit 1
fi

echo "=== D-04 parsing sortie CGI sur le port $PORT ==="
check_code /cgi-bin/normal.py 200 "sortie normale -> 200"
check_header /cgi-bin/normal.py "Content-Length: 5" "Content-Length calcule"
check_body_has /cgi-bin/normal.py "hello" "body normal transmis"

check_code /cgi-bin/empty_body.py 200 "headers valides + body vide -> 200"
check_header /cgi-bin/empty_body.py "Content-Length: 0" "body vide -> Content-Length 0"

check_code /cgi-bin/status.py 404 "Status CGI -> 404"
check_header /cgi-bin/status.py "Content-Type: text/plain" "Status CGI sans fuite du header Status"
check_body_has /cgi-bin/status.py "not found" "body du Status CGI transmis"

check_code /cgi-bin/redirect.py 302 "Location seule -> 302"
check_header /cgi-bin/redirect.py "Location: /index.html" "Location conservee"

check_code /cgi-bin/bad_status.py 502 "Status non numerique -> 502"
check_code /cgi-bin/out_of_range.py 502 "Status hors plage -> 502"
check_code /cgi-bin/no_headers.py 502 "sortie sans headers -> 502"
check_code /cgi-bin/no_output.py 502 "sortie vide -> 502"

echo
echo "Resume : $PASS reussi(s), $FAIL echoue(s)"
[ "$FAIL" -eq 0 ]
