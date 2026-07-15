# 09 — Tests et debug

Le sujet est explicite : « Do not test with only one program. Write your tests in a more suitable language, such as Python or Golang. » Un correcteur qui voit un `tests/` sérieux vous fait confiance sur le reste.

## 1. Les outils, par ordre de brutalité

| Outil | Ce qu'il t'apprend |
|---|---|
| `telnet` | Le HTTP brut, tapé à la main. Le plus pédagogique. |
| `nc` | Comme telnet mais scriptable, et il gère le binaire |
| `curl -v` | Ta requête et ta réponse, headers compris |
| Le navigateur | La vérité. Il fait des trucs que curl ne fait pas. |
| `nginx` | Ta référence de comportement |
| Python `socket` | Les cas tordus : fragmentation, envoi lent, malformé |
| `siege` / `ab` | La charge |
| `valgrind` | Les fuites |
| `strace` / `ltrace` | Les syscalls, quand tu ne comprends plus rien |
| `ss` / `lsof` | Les fds et les états TCP |

## 2. telnet — le HTTP à la main

```bash
telnet localhost 8080
GET / HTTP/1.1
Host: localhost
[entrée sur une ligne vide]
```

Fais-le au moins une fois pour de vrai. Voir la réponse ASCII arriver dans ton terminal, c'est ce qui rend le protocole concret.

Limite : telnet ne fait pas le binaire, et la ligne vide finale est capricieuse selon les clients. Pour tout le reste, `nc`.

## 3. curl — les invocations à connaître

```bash
curl -v http://localhost:8080/                    # headers requête + réponse
curl -I http://localhost:8080/                    # HEAD
curl --http1.0 http://localhost:8080/             # forcer 1.0
curl -X DELETE http://localhost:8080/uploads/a.txt
curl -X POST -d "name=jean&age=25" http://localhost:8080/cgi-bin/form.py
curl -F "file=@photo.png" http://localhost:8080/upload      # multipart
curl -H "Transfer-Encoding: chunked" --data-binary @big.txt http://localhost:8080/up
curl -X PUT http://localhost:8080/                # doit donner 501
curl --limit-rate 100 http://localhost:8080/big   # client lent
curl -o /dev/null -w "%{http_code} %{size_download}\n" http://localhost:8080/
```

Les deux qui trouvent des bugs :
- `--limit-rate 100` sur un gros fichier : force les envois partiels de `send`. Si ton fichier arrive tronqué, tu ne gères pas le retour partiel.
- `-F` avec un binaire de plusieurs Mo, puis `diff` : le seul vrai test du multipart.

## 4. nginx — la référence

```bash
docker run --rm -p 8081:80 -v $(pwd)/www:/usr/share/nginx/html:ro nginx
```

Compare, ligne par ligne, ces dix comportements :

| Cas | Regarde |
|---|---|
| `GET /` | Headers, ordre, format de `Date` |
| `GET /nexistepas` | Le code, le body |
| `GET /dir` (sans slash, dir existe) | **301 vers `/dir/`** |
| `GET /dir/` sans index, autoindex off | **403**, pas 404 |
| `PUT /` | 405 chez nginx (config), 501 chez toi si non implémentée |
| `GET / HTTP/1.1` sans Host | **400** |
| `GET / HTTP/9.9` | 400 ou 505 |
| Requête de 100 Ko de headers | 431 ou 400 |
| Body > `client_max_body_size` | **413** |
| Connexion ouverte sans rien envoyer | Fermée après ~60 s |

**Le sujet t'avertit** : « pay attention to differences between HTTP versions ». nginx en 1.1 ne fait pas la même chose qu'en 1.0. Compare à version égale.

Tu n'as **pas** à être identique à nginx. Tu dois savoir **où** tu diffères et **pourquoi**. C'est ça que le correcteur teste. « nginx fait 301, nous aussi » et « nginx fait X, nous faisons Y parce que le sujet dit Z » sont deux bonnes réponses. « Ah bon ? » n'en est pas une.

## 5. Les tests Python

`tests/run.py` — commence par ça, dès le jour 3 :

```python
#!/usr/bin/env python3
import socket, time, subprocess, sys

HOST, PORT = "127.0.0.1", 8080
results = []

def raw(payload, timeout=2.0, chunks=None, delay=0):
    """Envoie du brut, rend la réponse. chunks: taille des morceaux."""
    s = socket.create_connection((HOST, PORT), timeout=timeout)
    if chunks:
        for i in range(0, len(payload), chunks):
            s.sendall(payload[i:i+chunks])
            time.sleep(delay)
    else:
        s.sendall(payload)
    data = b""
    try:
        while True:
            b = s.recv(4096)
            if not b: break
            data += b
    except socket.timeout:
        pass
    s.close()
    return data

def status(resp):
    if not resp: return None
    try:    return int(resp.split(b"\r\n")[0].split(b" ")[1])
    except: return None

def check(name, resp, expected):
    got = status(resp)
    ok = got == expected
    results.append((name, ok, f"attendu {expected}, reçu {got}"))
    print(("  OK  " if ok else " FAIL ") + name + ("" if ok else f"  ({got} != {expected})"))

# --- parsing
check("GET simple",        raw(b"GET / HTTP/1.1\r\nHost: x\r\n\r\n"), 200)
check("pas de Host en 1.1",raw(b"GET / HTTP/1.1\r\n\r\n"), 400)
check("methode inconnue",  raw(b"BREW / HTTP/1.1\r\nHost: x\r\n\r\n"), 501)
check("version bidon",     raw(b"GET / HTTP/9.9\r\nHost: x\r\n\r\n"), 505)
check("request line vide", raw(b"\r\n\r\n"), 400)
check("double espace",     raw(b"GET  /  HTTP/1.1\r\nHost: x\r\n\r\n"), 400)
check("URI 9000 octets",   raw(b"GET /" + b"a"*9000 + b" HTTP/1.1\r\nHost: x\r\n\r\n"), 414)
check("espace avant :",    raw(b"GET / HTTP/1.1\r\nHost : x\r\n\r\n"), 400)
check("CL non numerique",  raw(b"POST /up HTTP/1.1\r\nHost: x\r\nContent-Length: abc\r\n\r\n"), 400)
check("CL negatif",        raw(b"POST /up HTTP/1.1\r\nHost: x\r\nContent-Length: -5\r\n\r\n"), 400)
check("CL + TE",           raw(b"POST /up HTTP/1.1\r\nHost: x\r\nContent-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\n"), 400)
check("2 CL differents",   raw(b"POST /up HTTP/1.1\r\nHost: x\r\nContent-Length: 5\r\nContent-Length: 6\r\n\r\n"), 400)

# --- fragmentation : LE test qui compte
req = b"GET / HTTP/1.1\r\nHost: x\r\n\r\n"
check("octet par octet", raw(req, chunks=1, delay=0.01), 200)
check("par 3",           raw(req, chunks=3, delay=0.01), 200)

# --- chunked
ck = (b"POST /cgi-bin/echo.py HTTP/1.1\r\nHost: x\r\n"
      b"Transfer-Encoding: chunked\r\n\r\n"
      b"7\r\nMozilla\r\n9\r\nDeveloper\r\n0\r\n\r\n")
r = raw(ck)
check("chunked", r, 200)
results.append(("chunked body == MozillaDeveloper", b"MozillaDeveloper" in r, ""))
check("chunk size non hexa", raw(b"POST /up HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\nZZ\r\n"), 400)

# --- limites
check("413", raw(b"POST /up HTTP/1.1\r\nHost: x\r\nContent-Length: 999999999\r\n\r\n"), 413)

# --- pipelining
r = raw(b"GET / HTTP/1.1\r\nHost: x\r\n\r\nGET / HTTP/1.1\r\nHost: x\r\n\r\n")
results.append(("pipelining : 2 reponses", r.count(b"HTTP/1.1") >= 2, ""))

# --- slowloris : le serveur doit rester dispo
socks = []
for _ in range(50):
    try:
        s = socket.create_connection((HOST, PORT), timeout=1)
        s.sendall(b"GET / HTTP/1.1\r\n")
        socks.append(s)
    except: pass
try:
    r = raw(b"GET / HTTP/1.1\r\nHost: x\r\n\r\n", timeout=3)
    results.append(("dispo sous slowloris", status(r) == 200, ""))
except Exception as e:
    results.append(("dispo sous slowloris", False, str(e)))
for s in socks: s.close()

# --- deconnexion brutale pendant l'envoi
s = socket.create_connection((HOST, PORT))
s.sendall(b"POST /up HTTP/1.1\r\nHost: x\r\nContent-Length: 1000000\r\n\r\n")
s.sendall(b"a" * 100)
s.close()                                   # RST
time.sleep(0.3)
r = raw(b"GET / HTTP/1.1\r\nHost: x\r\n\r\n")
results.append(("survit a un RST", status(r) == 200, ""))

failed = [r for r in results if not r[1]]
print(f"\n{len(results)-len(failed)}/{len(results)} OK")
sys.exit(1 if failed else 0)
```

Cinquante lignes, et il attrape 80% des bugs. Écris-le **avant** d'avoir fini, pas après.

Les trois tests qui trouvent le plus :
1. **La fragmentation octet par octet.** Si ton parser n'est pas incrémental, il meurt ici. Immédiatement.
2. **Le slowloris.** 50 connexions à moitié ouvertes. Si le 51e client ne peut plus se connecter, tu es vulnérable.
3. **Le RST pendant l'envoi.** Si ton serveur meurt, tu as oublié `SIGPIPE`.

## 6. Le stress test

```bash
siege -b -c 50 -t 30S http://localhost:8080/
```
- `-b` : benchmark, pas de délai entre requêtes
- `-c 50` : 50 clients simultanés
- `-t 30S` : pendant 30 secondes

Ce que tu regardes :

| Métrique | Cible |
|---|---|
| **Availability** | **100,00 %**. Le sujet dit « must remain available at all times ». 99,9 % = un bug. |
| Failed transactions | **0** |
| RAM (`top`, colonne RES) | Stable. Si ça monte linéairement, tu fuis. |
| fds (`ls /proc/$(pgrep webserv)/fd | wc -l`) | Se stabilise. Sinon tu fuis. |
| CPU au repos après le test | **0,0 %**. Sinon POLLOUT toujours armé. |

En parallèle du siege :
```bash
watch -n1 'ls /proc/$(pgrep webserv)/fd | wc -l; ps -o rss= -p $(pgrep webserv)'
```

Alternative : `ab -n 10000 -c 100 http://localhost:8080/`. Plus simple, moins réaliste.

Et le test qui compte vraiment : **relance le siege trois fois de suite**. Un serveur qui tient le premier et meurt au troisième a une fuite lente. C'est exactement ce que fait un correcteur méticuleux.

## 7. valgrind

```bash
valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes ./webserv conf/default.conf
```

`--track-fds=yes` est le plus utile ici : il liste les fds encore ouverts à la sortie. Les 3 premiers (0, 1, 2) sont normaux, tout le reste est une fuite.

Fais-le tourner, lance ta suite de tests, puis Ctrl-C (d'où l'intérêt de ton `SIGINT` handler : sans lui, valgrind ne peut pas faire son rapport de fin proprement).

**Le point sur les fuites.** Le sujet ne les interdit pas explicitement, mais :
- une fuite par requête = un serveur qui meurt après quelques heures ;
- « still reachable » à la sortie est acceptable (des objets globaux jamais libérés) ;
- « definitely lost » qui grandit à chaque requête = un vrai problème, et le correcteur qui lance valgrind le verra.

Alternative moderne : `-fsanitize=address` à la compilation. Plus rapide que valgrind, mais ne se combine pas avec `-Werror` sans bagarre, et pas dispo partout à 42. Valgrind reste plus sûr.

## 8. strace

Quand tu ne comprends plus rien :

```bash
strace -f -e trace=network,poll,read,write ./webserv conf/default.conf
```
Tu vois exactement ce que fait ton process. C'est **imparable** pour prouver (ou réfuter) un « je ne bloque nulle part ».

```bash
strace -c ./webserv conf/default.conf
# lance ta suite, Ctrl-C : tu as le compte des syscalls
```
Si `poll` a été appelé 4 millions de fois pour 100 requêtes, tu as ton bug de busy-loop. Chiffré, incontestable.

`-f` suit les forks — indispensable pour débuguer le CGI.

## 9. lsof et ss

```bash
lsof -p $(pgrep webserv)                 # tous les fds, avec leur nature
ss -tan | grep 8080                      # états TCP
ss -tan state time-wait | wc -l          # les TIME_WAIT qui s'accumulent
ls -l /proc/$(pgrep webserv)/fd          # le plus rapide
```

`lsof` te dit **quoi** fuit : des sockets ? des fichiers ? des pipes ? Si tu vois des pipes qui s'accumulent, ton CGI ne ferme pas. Si tu vois des `.html` ouverts, tu as un `open` sans `close` dans le chemin d'erreur de `serveFile`.

## 10. Le tester du sujet

Le sujet mentionne « We have provided a small tester ». Il n'est pas obligatoire, mais lance-le : c'est possiblement ce que le correcteur lancera.

Ne cours pas après le 100% si tu ne comprends pas un cas. Un tester qui teste un comportement non spécifié n'est pas la loi — le sujet l'est. Mais chaque échec mérite un « j'ai compris pourquoi, et voilà notre choix ».

## 11. Le protocole de test avant chaque merge

Cinq minutes, à chaque fois :

```bash
make re                            # compile propre, zéro warning
./webserv conf/bad/no_listen.conf  # erreur claire + exit 1
./webserv conf/default.conf &
python3 tests/run.py               # tout vert
curl -F "file=@test.png" localhost:8080/upload && diff test.png www/uploads/test.png
siege -b -c 20 -t 10S localhost:8080/    # 100% availability
ls /proc/$(pgrep webserv)/fd | wc -l     # stable
# navigateur : le site s'affiche, CSS et images comprises
kill -INT %1                       # arrêt propre
```

Un script `tests/smoke.sh` avec tout ça dedans, et chacun le lance avant de pousser. C'est ce qui vous évite le « ça marchait sur ma branche ».

## 12. À retenir

- Teste la fragmentation octet par octet dès le début. C'est le test qui trouve le plus.
- Compare à nginx, à version HTTP égale, et **sache expliquer** chaque écart.
- 100,00 % availability au siege. Pas 99,9 %.
- 0,0 % CPU au repos.
- Compte les fds sous charge. Ils doivent se stabiliser.
- `valgrind --track-fds=yes`.
- `strace -c` prouve les busy-loops de façon incontestable.
- Le navigateur est la vérité finale. curl ne suffit pas.
- Relance le stress test trois fois.

## 13. Exercice

Écris `tests/run.py` avec les 20 cas du §5 **maintenant**, avant d'avoir un serveur qui répond. Ils vont tous échouer. C'est normal, c'est ta définition de « fini ». Chaque test qui passe au vert est un ticket fermé, et tu sais toujours exactement où tu en es.
