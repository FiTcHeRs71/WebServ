#!/usr/bin/env python3
"""Fuzzeur pour C-04 (erreurs de parsing -> 400/501/505, jamais de crash).

Rejoue une table de requetes malformees/limites + un lot de requetes
tronquees aleatoires, contre une instance de webserv lancee par ce script.
Pour chaque cas :
  - on ouvre une connexion neuve (une requete en erreur ferme le flux,
	Connection: close),
  - on envoie le payload, on lit la reponse (ou l'absence de reponse),
  - on verifie le code de statut attendu, et Connection: close si un code
	d'erreur est attendu,
  - on verifie que le process est toujours vivant apres.

A la fin : verification qu'aucun fd ne fuit (avant/apres), et si le script
tourne sous valgrind (option par defaut, voir --no-valgrind), lecture du
rapport pour confirmer 0 erreur et 0 octet "definitely lost".

Usage :
  ./tests/fuzz_requests.py                  # tout, sous valgrind
  ./tests/fuzz_requests.py --no-valgrind    # plus rapide, sans memcheck
  ./tests/fuzz_requests.py --port 8199
  ./tests/fuzz_requests.py --keep           # laisse le serveur tourner a la fin
  ./tests/fuzz_requests.py --random 500     # nombre de cas aleatoires (defaut 200)
"""

import argparse
import os
import random
import re
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import time

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(ROOT_DIR, "webserv")
HOST = "127.0.0.1"

STATUS_RE = re.compile(rb"^HTTP/1\.[01] (\d{3}) ")


# --------------------------------------------------------------------------
# Table de cas : (nom, payload, code attendu ou None si reponse non attendue)
# --------------------------------------------------------------------------
def build_cases():
	cases = []

	def add(name, payload, expected):
		cases.append((name, payload, expected))

	# --- 400 : requete mal formee -------------------------------------
	add("host_absent_zero_header", b"GET / HTTP/1.1\r\n\r\n", 400)
	add("host_absent_avec_autres_headers",
		b"GET / HTTP/1.1\r\nX-Foo: bar\r\n\r\n", 400)
	add("host_duplique", b"GET / HTTP/1.1\r\nHost: a\r\nHost: b\r\n\r\n", 400)
	add("methode_sans_espace", b"GET/HTTP/1.1\r\n\r\n", 400)
	add("methode_double_espace",
		b"GET  / HTTP/1.1\r\nHost: x\r\n\r\n", 400)
	add("path_sans_slash", b"GET x HTTP/1.1\r\nHost: x\r\n\r\n", 400)
	add("path_double_espace",
		b"GET /  HTTP/1.1\r\nHost: x\r\n\r\n", 400)
	add("percent_encoding_invalide",
		b"GET /%zz HTTP/1.1\r\nHost: x\r\n\r\n", 400)
	add("percent_encoding_tronque",
		b"GET /%4 HTTP/1.1\r\nHost: x\r\n\r\n", 400)
	add("header_sans_deux_points",
		b"GET / HTTP/1.1\r\nHost: x\r\nBadHeaderSansColon\r\n\r\n", 400)
	add("content_length_et_transfer_encoding",
		b"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 5\r\n"
		b"Transfer-Encoding: chunked\r\n\r\nhello", 400)
	add("content_length_non_numerique",
		b"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: abc\r\n\r\nhello", 400)
	add("content_length_negatif",
		b"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: -5\r\n\r\nhello", 400)
	add("chunk_size_non_hexa",
		b"POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n"
		b"ZZZ\r\nabc\r\n0\r\n\r\n", 400)
	add("chunk_crlf_absent",
		b"POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n"
		b"3\r\nabcXX0\r\n\r\n", 400)

	# --- 411 : POST sans Content-Length ni Transfer-Encoding -----------
	add("post_sans_content_length",
		b"POST / HTTP/1.1\r\nHost: x\r\n\r\n", 411)

	# --- 413 : corps trop gros (client_max_body_size=50 dans la conf) --
	add("body_trop_gros_content_length",
		b"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 500\r\n\r\n"
		+ b"A" * 500, 413)
	add("body_trop_gros_chunked",
		b"POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n"
		b"c8\r\n" + b"A" * 200 + b"\r\n0\r\n\r\n", 413)

	# --- 414 : URI trop longue ------------------------------------------
	add("uri_trop_longue",
		b"GET /" + b"a" * 9000 + b" HTTP/1.1\r\nHost: x\r\n\r\n", 414)

	# --- 431 : headers trop gros / trop nombreux -------------------------
	add("trop_de_headers",
		b"GET / HTTP/1.1\r\nHost: x\r\n"
		+ b"".join(b"X-%d: v\r\n" % i for i in range(150)) + b"\r\n", 431)
	add("headers_trop_gros",
		b"GET / HTTP/1.1\r\nHost: x\r\nX-Big: " + b"a" * 9000 + b"\r\n\r\n",
		431)

	# --- 501 : methode/mecanisme non implemente --------------------------
	add("methode_inconnue", b"BREW / HTTP/1.1\r\nHost: x\r\n\r\n", 501)
	add("methode_minuscule", b"get / HTTP/1.1\r\nHost: x\r\n\r\n", 501)
	add("transfer_encoding_inconnu",
		b"POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: gzip\r\n\r\n",
		501)

	# --- 505 : version HTTP non supportee ---------------------------------
	add("version_http_2", b"GET / HTTP/2.0\r\nHost: x\r\n\r\n", 505)
	add("version_http_09", b"GET / HTTP/0.9\r\nHost: x\r\n\r\n", 505)
	add("version_garbage", b"GET / FTP/1.1\r\nHost: x\r\n\r\n", 505)

	# --- Incomplet : ne doit produire AUCUNE reponse (juste une attente) -
	add("requete_ligne_tronquee", b"GET / HTTP/1.1", None)
	add("headers_jamais_termines",
		b"GET / HTTP/1.1\r\nHost: x", None)
	add("chunk_jamais_termine",
		b"POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n"
		b"5\r\nabc", None)

	return cases


# --------------------------------------------------------------------------
# Utilitaires
# --------------------------------------------------------------------------
def wait_ready(port, proc, timeout=3.0):
	"""Attend que le PROCESS QU'ON VIENT DE LANCER ecoute sur port.

	Ne pas se contenter de tester la connexion : un vieux process laisse en
	vie par un run precedent (--keep) peut deja occuper ce port, auquel cas
	la connexion reussirait alors que *notre* process a echoue au bind() et
	est deja mort. On verifie donc proc.poll() a chaque iteration - avec une
	courte pause initiale pour lui laisser le temps d'echouer si bind() doit
	echouer (sinon le tout premier poll() peut encore voir None, juste avant
	l'echec, pendant qu'une connexion reussit deja vers l'ancien listener) -
	et une revalidation juste apres un succes de connexion, pour le meme
	risque de course dans l'autre sens.
	"""
	time.sleep(0.15)
	deadline = time.time() + timeout
	while time.time() < deadline:
		if proc.poll() is not None:
			return False
		try:
			with socket.create_connection((HOST, port), timeout=0.2):
				pass
		except OSError:
			time.sleep(0.05)
			continue
		time.sleep(0.05)
		return proc.poll() is None
	return False


def send_and_recv(port, payload, timeout=1.5):
	"""Retourne (bytes_recus_ou_None, exception_ou_None)."""
	try:
		s = socket.create_connection((HOST, port), timeout=timeout)
	except OSError as e:
		return None, e
	try:
		s.sendall(payload)
		s.settimeout(timeout)
		data = b""
		try:
			while True:
				chunk = s.recv(65536)
				if not chunk:
					break
				data += chunk
		except socket.timeout:
			pass
		return data, None
	finally:
		s.close()


def status_of(data):
	if not data:
		return None
	m = STATUS_RE.match(data)
	return int(m.group(1)) if m else None


def has_connection_close(data):
	if not data:
		return False
	head = data.split(b"\r\n\r\n", 1)[0].lower()
	return b"connection: close" in head


def describe_death(returncode):
    """Traduit Popen.returncode en texte : negatif = tue par un signal."""
    if returncode is not None and returncode < 0:
        signum = -returncode
        try:
            name = signal.Signals(signum).name
        except ValueError:
            name = "?"
        return "tue par signal %d (%s)" % (signum, name)
    return "exit code %s" % returncode


def fd_count(pid):
	return len(os.listdir("/proc/%d/fd" % pid))


def random_payload():
	"""Requete tronquee/corrompue aleatoire : jamais de crash attendu."""
	base = random.choice([
		b"GET / HTTP/1.1\r\nHost: x\r\n\r\n",
		b"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 20\r\n\r\n" + b"A" * 20,
		b"GET / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n"
		b"5\r\nhello\r\n0\r\n\r\n",
	])
	n = random.randint(0, len(base))
	truncated = base[:n]
	if random.random() < 0.3:
		# injecte des octets aleatoires (potentiellement invalides en UTF-8)
		junk = bytes(random.randint(0, 255) for _ in range(random.randint(1, 30)))
		truncated += junk
	return truncated


# --------------------------------------------------------------------------
# Main
# --------------------------------------------------------------------------
def main():
	ap = argparse.ArgumentParser()
	ap.add_argument("--port", type=int, default=8199)
	ap.add_argument("--keep", action="store_true")
	ap.add_argument("--no-valgrind", action="store_true")
	ap.add_argument("--random", type=int, default=200,
					 help="nombre de requetes aleatoires tronquees (defaut 200)")
	args = ap.parse_args()

	if not os.path.isfile(BIN) or not os.access(BIN, os.X_OK):
		print("webserv introuvable ou non executable : %s (fais 'make' d'abord)" % BIN)
		return 2
	use_valgrind = not args.no_valgrind
	if use_valgrind and not shutil.which("valgrind"):
		print("valgrind introuvable, on continue sans (--no-valgrind pour enlever ce message)")
		use_valgrind = False

	conf_fd, conf_path = tempfile.mkstemp(suffix=".conf")
	os.write(conf_fd, (
		"server\n{\n\tlisten %d;\n\tclient_max_body_size 50;\n}\n" % args.port
	).encode())
	os.close(conf_fd)

	valgrind_log = tempfile.mktemp(suffix=".valgrind.log") if use_valgrind else None
	server_log = tempfile.mktemp(suffix=".server.log")

	if use_valgrind:
		cmd = ["valgrind", "--leak-check=full", "--show-leak-kinds=all",
			   "--track-origins=yes", "--log-file=" + valgrind_log,
			   BIN, conf_path]
	else:
		cmd = [BIN, conf_path]

	with open(server_log, "wb") as logf:
		proc = subprocess.Popen(cmd, stdout=logf, stderr=subprocess.STDOUT)

	def cleanup():
		if not args.keep:
			if proc.poll() is None:
				try:
					if use_valgrind:
						proc.send_signal(signal.SIGINT)
						proc.wait(timeout=10)
					else:
						proc.terminate()
						proc.wait(timeout=3)
				except Exception:
					try:
						proc.kill()
						proc.wait(timeout=3)
					except Exception:
						pass
			os.remove(conf_path)
			if os.path.exists(server_log):
				os.remove(server_log)
		else:
			print("serveur laisse en vie (PID %d), conf=%s log=%s"
				  % (proc.pid, conf_path, server_log))

	if not wait_ready(args.port, proc):
		died = proc.poll() is not None
		print("le serveur n'a pas demarre sur le port %d (%s) :"
		      % (args.port, describe_death(proc.returncode) if died else "timeout, jamais mort"))
		with open(server_log) as f:
			print(f.read())
		if died and "Address already in use" in open(server_log).read():
			print("-> un autre process ecoute deja sur ce port : essaie --port <autre>,")
			print("   ou verifie avec `pgrep -af webserv` s'il en reste un d'un run precedent.")
		cleanup()
		return 2

	print("webserv PID=%d port=%d valgrind=%s\n" % (proc.pid, args.port, use_valgrind))

	fails = []
	passed = 0

	server_died = False

	print("=== Table de cas (%d) ===" % len(build_cases()))
	for name, payload, expected in build_cases():
		data, err = send_and_recv(args.port, payload)
		if not proc.poll() is None:
			print("  %-38s >>> SERVEUR MORT (%s) <<<" % (name, describe_death(proc.returncode)))
			fails.append(name)
			server_died = True
			break
		if err is not None:
			print("  %-38s ERREUR connexion: %s" % (name, err))
			fails.append(name)
			continue
		got = status_of(data)
		if expected is None:
			ok = (got is None)
			detail = "pas de reponse (attendu)" if ok else "recu %r alors qu'aucune reponse n'etait attendue" % got
		else:
			close_ok = has_connection_close(data)
			ok = (got == expected) and close_ok
			if got != expected:
				detail = "attendu %d, recu %r" % (expected, got if got else data[:60])
			elif not close_ok:
				detail = "code %d correct mais 'Connection: close' absent" % got
			else:
				detail = "%d + Connection: close" % got
		print("  %-38s %-4s %s" % (name, "OK" if ok else "FAIL", detail))
		if ok:
			passed += 1
		else:
			fails.append(name)

	if server_died:
		print("\nServeur deja mort, on saute le fuzzing aleatoire et le check de fd.")
	else:
		print("\n=== %d requetes aleatoires tronquees/corrompues ===" % args.random)
		print("  (la plupart n'obtiennent aucune reponse, c'est attendu : chaque cas")
		print("   attend jusqu'a 0.15s avant de conclure, ~%.0fs max au total)"
		      % (args.random * 0.15))
		before_fd = fd_count(proc.pid)
		random_fail = 0
		for i in range(args.random):
			if i % 20 == 0:
				sys.stdout.write("  [%d/%d]\r" % (i, args.random))
				sys.stdout.flush()
			payload = random_payload()
			data, err = send_and_recv(args.port, payload, timeout=0.15)
			if proc.poll() is not None:
				print("  >>> SERVEUR MORT (%s) sur le cas aleatoire #%d (payload=%r) <<<"
				      % (describe_death(proc.returncode), i, payload))
				fails.append("random_%d" % i)
				random_fail += 1
				server_died = True
				break
			if data:
				got = status_of(data)
				if got is None and data:
					print("  >>> reponse non-HTTP sur #%d : %r <<<" % (i, data[:60]))
					random_fail += 1
		if random_fail == 0:
			print("  %d cas rejoues, aucun crash, aucune reponse malformee." % args.random)
			passed += 1
		else:
			fails.append("fuzz_aleatoire")

		if not server_died:
			print("\n=== Fuite de fd ===")
			time.sleep(0.3)
			after_fd = fd_count(proc.pid)
			if after_fd <= before_fd:
				print("  avant=%d apres=%d  OK" % (before_fd, after_fd))
				passed += 1
			else:
				print("  avant=%d apres=%d  >>> FUITE +%d <<<" % (before_fd, after_fd, after_fd - before_fd))
				fails.append("fd_leak")

	still_alive = (not server_died) and proc.poll() is None
	print("\nserveur toujours vivant : %s" % ("oui" if still_alive else "NON -- CRASH"))
	if not still_alive:
		fails.append("process_alive")
		print("\n--- log serveur ---")
		try:
			with open(server_log) as f:
				print(f.read())
		except OSError:
			pass

	cleanup()

	if args.keep:
		print("\n--keep actif : rapport valgrind pas encore finalise (process laisse en vie), analyse sautee.")
	elif use_valgrind and valgrind_log and os.path.exists(valgrind_log):
		print("\n=== Rapport valgrind ===")
		with open(valgrind_log) as f:
			content = f.read()
		m_err = re.search(r"ERROR SUMMARY: (\d+) errors", content)
		m_lost = re.search(r"definitely lost: ([\d,]+) bytes", content)
		errors = int(m_err.group(1)) if m_err else -1
		lost = int(m_lost.group(1).replace(",", "")) if m_lost else -1
		print("  ERROR SUMMARY: %s" % (errors if errors >= 0 else "introuvable (log incomplet ?)"))
		print("  definitely lost: %s bytes" % (lost if lost >= 0 else "introuvable"))
		if errors < 0 or lost < 0:
			print("  log incomplet (le process a peut-etre ete tue avant la fin de l'analyse) : %s" % valgrind_log)
			fails.append("valgrind_log_incomplet")
		elif errors != 0 or lost != 0:
			print("  log complet: %s" % valgrind_log)
			fails.append("valgrind")
		else:
			os.remove(valgrind_log)

	print("\n=== Resume : %d reussi(s), %d echoue(s) ===" % (passed, len(fails)))
	if fails:
		print("Echecs : " + ", ".join(fails))
		return 1
	return 0


if __name__ == "__main__":
	sys.exit(main())
