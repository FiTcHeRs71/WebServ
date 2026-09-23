# Webserv : HTTP/1.1 server in C++98

*This project has been created as part of the 42 curriculum by fducrot, lgranger, ludebarn.*

*[English](#english) · [Français](#français)*

---

## English

Webserv is an **HTTP/1.1 server written in C++98**, with no external library.
The goal is not to use a web server but to build one: sockets, non blocking
I/O, request parsing, response generation and CGI execution.

The whole server runs on a **single `poll()` loop** that watches every listening
socket, every client socket and every CGI pipe, so one slow client never blocks
the others. Its behavior is driven by a **configuration file inspired by NGINX**.

### ⚙️ Build constraints

```bash
c++ -Wall -Wextra -Werror -std=c++98
```

Rules imposed by the subject:
- **C++98 only**, no external library, no Boost.
- Every class follows the canonical form (constructor, copy, assignment,
  destructor).
- The server must **never block** and must **never die**, whatever the client does.
- `poll()` (or equivalent) is the only allowed multiplexing primitive, and it
  must handle read and write at once.
- No `errno` checking after a read or a write outside of the multiplexer.

### 🚀 Build and run

```bash
make          # build ./webserv
make clean    # remove objects
make fclean   # remove objects + binary
make re       # rebuild
make run      # build and run
```

Run with a configuration file:

```bash
./webserv conf/default.conf
```

Then open <http://localhost:8080> in a browser, or use curl:

```bash
curl -i http://localhost:8080/
curl -i -X POST --data-binary @file.txt http://localhost:8080/uploads/file.txt
curl -i -X DELETE http://localhost:8080/uploads/file.txt
```

Requirements: a C++ compiler (`c++` or `clang++`), `make`, `python3` and
`php-cgi` for the CGI examples.

### 🧩 Key concepts

- **HTTP request**: a text message sent by a client. It starts with a request
  line (method, path, version, for example `GET /index.html HTTP/1.1`), then
  headers (`Host`, `Content-Length`, `Cookie`), an empty line and an optional
  body. The server answers with a status line (`HTTP/1.1 200 OK`), headers and
  a body.
- **Socket**: an endpoint for network communication, seen by the program as a
  file descriptor. The server opens one listening socket per address and port,
  then gets one new socket for each client it accepts.
- **Non blocking I/O with poll**: instead of waiting on one client, the server
  asks `poll()` which sockets and pipes are ready, and only reads or writes on
  those. This is how a single process serves many clients at the same time.
- **CGI (Common Gateway Interface)**: a standard way to let an external program
  build the response. For a file like `hello.py`, the server starts the
  interpreter in a child process, passes the request data through environment
  variables and the body on stdin, then reads the headers and the page from its
  stdout.
- **NGINX**: a widely used real web server. Webserv copies the style of its
  configuration file (`server` and `location` blocks, `listen`, `root`, `index`)
  and uses its behavior as the reference.
- **WebSocket**: a different protocol keeping a two way channel open between a
  browser and a server. It starts as an HTTP request but is out of scope here.
  Webserv only speaks HTTP/1.1, one request and one response at a time.

### 📚 Features

| Area | What is implemented |
|------|---------------------|
| Methods | `GET`, `POST`, `DELETE` |
| Static | root, index files, directory listing (`autoindex`) |
| Errors | default error pages and custom `error_page` |
| Upload | file upload into a configurable `upload_store` |
| Body | `client_max_body_size` limit and chunked request bodies |
| Redirect | `return` directive (3xx with target, or bare status) |
| Hosting | several servers, several ports, virtual hosts (`server_name`) |
| CGI | execution based on the file extension (`cgi_ext` / `cgi_pass`) |
| Logging | log file with timestamped server events |

### 🗂️ Project structure

```
webServ/
├── srcs/
│   ├── main.cpp
│   ├── config/      # config file parsing, listen, server resolution
│   ├── network/     # listening sockets, poll loop, connections
│   ├── http/        # request, response, router, autoindex
│   ├── cgi/         # child process, pipes, CGI environment
│   └── utils/       # logger
├── includes/        # one header per class
├── conf/            # working configs + conf/bad for parser tests
├── www/             # site root, cgi-bin, errors, uploads
├── tests/           # shell and python test scripts
├── docs/            # design notes, diagrams, Doxygen
└── Makefile
```

Reading order to understand the flow: `main.cpp` → `Config` → `ListenSockets`
→ `EventLoop` → `Connection` → `Request` → `Router` → `Response` (and
`CgiProcess` when the route is a CGI).

### 📖 Configuration

Each `server` block accepts `listen`, `server_name`, `client_max_body_size`,
`error_page` and `location` blocks. A `location` accepts `root`, `index`,
`autoindex`, `allow_methods`, `return`, `upload_store`, `cgi_ext` and `cgi_pass`.

```nginx
server {
    listen                  0.0.0.0:8080;
    listen                  8081;          # port only, binds 0.0.0.0:8081
    listen                  *:9090;        # wildcard, binds 0.0.0.0:9090
    server_name             webserv;
    client_max_body_size    10M;

    error_page 404          /errors/404.html;

    location / {
        allow_methods       GET;
        root                ./www;
        index               index.html;
    }

    location /cgi-bin {
        allow_methods       GET POST;
        root                ./www/cgi-bin;
        cgi_ext             .py;
        cgi_pass            /usr/bin/python3;
    }

    location /uploads {
        allow_methods       GET POST DELETE;
        root                ./www/uploads;
        autoindex           on;
        upload_store        ./www/uploads;
    }
}
```

Two `server` blocks may share the same `host:port` as long as their
`server_name` differ: that is the virtual host. On a given port, `0.0.0.0` and
a precise address cannot coexist, because the kernel refuses the second
`bind()`.

Commented examples live in `conf/default.conf` and `docs/webserv-fichier-conf.md`.
Invalid files used to test the parser are in `conf/bad`.

### 🧪 Demo page

`www/index.html` is an interactive HTTP playground served by webserv itself:
it lists the files in `/uploads`, lets you upload and delete them from the
browser, logs every exchange (method, path, status, duration) and exposes the
other routes (`/old` redirect, `/deny`, `/ping`, `/session`, CGI scripts).

### ✨ Bonus

**Cookies and sessions.** The server parses the `Cookie` header of each
request, passes it to CGI scripts as `HTTP_COOKIE`, and forwards every
`Set-Cookie` header sent back by a CGI. The example `www/cgi-bin/cookie.py`
gives a new client a `sessionid` and counts its visits. Two clients get two
distinct sessions.

```bash
curl -i -c jar -b jar http://localhost:8080/cgi-bin/cookie.py
```

Run it twice: the first answer sets the cookie, the second one shows the visit
counter increasing.

The server also handles sessions **natively, without any CGI**, on the
`/session` route. On the first visit it creates a random `sessionid` (16 bytes
read from `/dev/urandom`), keeps it in memory and sends it back in a
`Set-Cookie` header (`Path=/`, `HttpOnly`). On each next visit it finds the
session from the cookie and increases its counter. An unknown or forged
`sessionid` gets a new session. Sessions are lost when the server stops.

```bash
curl -i -c jar -b jar http://localhost:8080/session
```

**Multiple CGI types.** One location can map several extensions to several
interpreters, for example `.py` to `python3` and `.php` to `php-cgi`.

```bash
curl -i http://localhost:8080/cgi-bin/hello.py
curl -i http://localhost:8080/cgi-bin/hello.php
```

### 📝 Logger

At startup the server output is redirected to `./webserv.log`, so the terminal
stays clean. Each line is timestamped with a level (`info`, `error`). It records
the startup, the listening addresses, accepted and closed connections, CGI
starts, the shutdown, and system call failures such as `poll` or `accept`. If
the file cannot be opened, logging falls back to the terminal.

```
11/09/2026 08:42:21 [info] accept 127.0.0.1:55348 on fd 7
11/09/2026 08:42:21 [info] GET /cgi-bin/cookie.py -> cgi started
11/09/2026 08:42:21 [info] close 127.0.0.1 on fd 7
```

Follow it while the server runs:

```bash
tail -f webserv.log
```

### 🔬 Tests

Shell and Python scripts are in `tests/`: config parsing, listening sockets,
autoindex, DELETE, CGI output and lifecycle, file descriptor leaks, and a
request fuzzer. The 42 tester runs with `conf/tester.conf`.

```bash
./tests/test_conf.sh
./tests/test_autoindex.sh
python3 tests/fuzz_requests.py
```

File descriptors are checked with valgrind, which catches both leaks and double
closes, especially in the CGI child:

```bash
valgrind --track-fds=yes --trace-children=no ./webserv conf/default.conf
```

### 🧠 About curl

`curl` is a command line HTTP client. It sends one request to the given URL and
prints the answer, which makes it handy to test the server without a browser.
Options used in this README:

- `-i`: also print the status line and the response headers, not only the body.
- `-X METHOD`: choose the request method (`GET` is the default).
- `--data-binary @file`: send the file content as the request body, as is. This
  also turns the request into a `POST`.
- `-c jar`: save the cookies set by the server (`Set-Cookie`) into a file named
  `jar`. The name is free, jar stands for cookie jar.
- `-b jar`: read the cookies stored in `jar` and send them in a `Cookie` header.
- `-v`: verbose mode, also print the request headers sent by curl.

Together, `-c jar -b jar` make curl behave like a browser: the first request has
no cookie and gets one, the next requests send it back. Run `cat jar` to see the
stored `sessionid`.

### 📎 Resources

- RFC 9110, HTTP Semantics
- RFC 9112, HTTP/1.1
- RFC 3875, The Common Gateway Interface
- RFC 6265, HTTP State Management (cookies)
- NGINX documentation, nginx.org/en/docs
- Beej Guide to Network Programming
- MDN Web Docs, HTTP section
- Linux manual pages: `socket`, `poll`, `fork`, `execve`, `pipe`

---

## Français

Webserv est un **serveur HTTP/1.1 écrit en C++98**, sans aucune bibliothèque
externe. Le but n'est pas d'utiliser un serveur web mais d'en construire un :
sockets, I/O non bloquantes, parsing des requêtes, génération des réponses et
exécution de CGI.

Tout le serveur tourne sur une **unique boucle `poll()`** qui surveille chaque
socket d'écoute, chaque socket client et chaque pipe de CGI : un client lent ne
bloque donc jamais les autres. Son comportement est piloté par un **fichier de
configuration inspiré de NGINX**.

### ⚙️ Contraintes de compilation

```bash
c++ -Wall -Wextra -Werror -std=c++98
```

Règles imposées par le sujet :
- **C++98 uniquement**, pas de bibliothèque externe, pas de Boost.
- Chaque classe respecte la forme canonique (constructeur, copie, affectation,
  destructeur).
- Le serveur ne doit **jamais bloquer** ni **jamais mourir**, quoi que fasse le
  client.
- `poll()` (ou équivalent) est la seule primitive de multiplexage autorisée, et
  elle doit gérer lecture et écriture en même temps.
- Pas de vérification d'`errno` après une lecture ou une écriture en dehors du
  multiplexeur.

### 🚀 Compilation et lancement

```bash
make          # compile ./webserv
make clean    # supprime les objets
make fclean   # supprime objets + binaire
make re       # recompile
make run      # compile et lance
```

Lancement avec un fichier de configuration :

```bash
./webserv conf/default.conf
```

Puis ouvrir <http://localhost:8080> dans un navigateur, ou utiliser curl :

```bash
curl -i http://localhost:8080/
curl -i -X POST --data-binary @file.txt http://localhost:8080/uploads/file.txt
curl -i -X DELETE http://localhost:8080/uploads/file.txt
```

Prérequis : un compilateur C++ (`c++` ou `clang++`), `make`, `python3` et
`php-cgi` pour les exemples CGI.

### 🧩 Notions clés

- **Requête HTTP** : un message texte envoyé par un client. Elle commence par
  une ligne de requête (méthode, chemin, version, par exemple
  `GET /index.html HTTP/1.1`), puis des en-têtes (`Host`, `Content-Length`,
  `Cookie`), une ligne vide et un corps optionnel. Le serveur répond par une
  ligne de statut (`HTTP/1.1 200 OK`), des en-têtes et un corps.
- **Socket** : un point de communication réseau, vu par le programme comme un
  descripteur de fichier. Le serveur ouvre une socket d'écoute par couple
  adresse/port, puis obtient une nouvelle socket pour chaque client accepté.
- **I/O non bloquantes avec poll** : au lieu d'attendre sur un client, le
  serveur demande à `poll()` quelles sockets et quels pipes sont prêts, et ne
  lit ou n'écrit que sur ceux-là. C'est ainsi qu'un seul processus sert
  plusieurs clients en même temps.
- **CGI (Common Gateway Interface)** : une norme qui laisse un programme externe
  construire la réponse. Pour un fichier comme `hello.py`, le serveur lance
  l'interpréteur dans un processus fils, lui passe les données de la requête via
  des variables d'environnement et le corps sur son stdin, puis lit les en-têtes
  et la page sur son stdout.
- **NGINX** : un vrai serveur web très répandu. Webserv reprend le style de son
  fichier de configuration (blocs `server` et `location`, `listen`, `root`,
  `index`) et prend son comportement comme référence.
- **WebSocket** : un autre protocole, qui garde un canal bidirectionnel ouvert
  entre un navigateur et un serveur. Il démarre par une requête HTTP mais reste
  hors sujet ici. Webserv ne parle que HTTP/1.1, une requête et une réponse à la
  fois.

### 📚 Fonctionnalités

| Domaine | Ce qui est implémenté |
|---------|-----------------------|
| Méthodes | `GET`, `POST`, `DELETE` |
| Statique | root, fichiers d'index, listing de répertoire (`autoindex`) |
| Erreurs | pages d'erreur par défaut et `error_page` personnalisées |
| Upload | envoi de fichiers dans un `upload_store` configurable |
| Corps | limite `client_max_body_size` et corps en chunked |
| Redirection | directive `return` (3xx avec cible, ou statut seul) |
| Hébergement | plusieurs serveurs, plusieurs ports, virtual hosts (`server_name`) |
| CGI | exécution selon l'extension (`cgi_ext` / `cgi_pass`) |
| Journal | fichier de log avec événements horodatés |

### 🗂️ Structure du projet

```
webServ/
├── srcs/
│   ├── main.cpp
│   ├── config/      # parsing de la conf, listen, résolution des serveurs
│   ├── network/     # sockets d'écoute, boucle poll, connexions
│   ├── http/        # requête, réponse, routeur, autoindex
│   ├── cgi/         # processus fils, pipes, environnement CGI
│   └── utils/       # logger
├── includes/        # un header par classe
├── conf/            # configs valides + conf/bad pour tester le parseur
├── www/             # racine du site, cgi-bin, errors, uploads
├── tests/           # scripts shell et python
├── docs/            # notes de conception, diagrammes, Doxygen
└── Makefile
```

Ordre de lecture pour suivre le flux : `main.cpp` → `Config` → `ListenSockets`
→ `EventLoop` → `Connection` → `Request` → `Router` → `Response` (et
`CgiProcess` quand la route est un CGI).

### 📖 Configuration

Chaque bloc `server` accepte `listen`, `server_name`, `client_max_body_size`,
`error_page` et des blocs `location`. Une `location` accepte `root`, `index`,
`autoindex`, `allow_methods`, `return`, `upload_store`, `cgi_ext` et `cgi_pass`.

```nginx
server {
    listen                  0.0.0.0:8080;
    listen                  8081;          # port seul, bind 0.0.0.0:8081
    listen                  *:9090;        # wildcard, bind 0.0.0.0:9090
    server_name             webserv;
    client_max_body_size    10M;

    error_page 404          /errors/404.html;

    location / {
        allow_methods       GET;
        root                ./www;
        index               index.html;
    }

    location /cgi-bin {
        allow_methods       GET POST;
        root                ./www/cgi-bin;
        cgi_ext             .py;
        cgi_pass            /usr/bin/python3;
    }

    location /uploads {
        allow_methods       GET POST DELETE;
        root                ./www/uploads;
        autoindex           on;
        upload_store        ./www/uploads;
    }
}
```

Deux blocs `server` peuvent partager le même `host:port` tant que leurs
`server_name` diffèrent : c'est le virtual host. Sur un même port, `0.0.0.0` et
une adresse précise ne peuvent pas coexister, le noyau refusant le second
`bind()`.

Des exemples commentés sont dans `conf/default.conf` et
`docs/webserv-fichier-conf.md`. Les fichiers invalides servant à tester le
parseur sont dans `conf/bad`.

### 🧪 Page de démonstration

`www/index.html` est un terrain de jeu HTTP interactif servi par webserv
lui-même : il liste les fichiers de `/uploads`, permet de les envoyer et de les
supprimer depuis le navigateur, journalise chaque échange (méthode, chemin,
statut, durée) et expose les autres routes (redirection `/old`, `/deny`,
`/ping`, `/session`, scripts CGI).

### ✨ Bonus

**Cookies et sessions.** Le serveur lit l'en-tête `Cookie` de chaque requête, le
transmet aux scripts CGI via `HTTP_COOKIE`, et renvoie tel quel chaque
`Set-Cookie` produit par un CGI. L'exemple `www/cgi-bin/cookie.py` attribue un
`sessionid` à un nouveau client et compte ses visites. Deux clients obtiennent
deux sessions distinctes.

```bash
curl -i -c jar -b jar http://localhost:8080/cgi-bin/cookie.py
```

À lancer deux fois : la première réponse pose le cookie, la seconde montre le
compteur de visites augmenter.

Le serveur gère aussi les sessions **nativement, sans CGI**, sur la route
`/session`. À la première visite il crée un `sessionid` aléatoire (16 octets lus
dans `/dev/urandom`), le garde en mémoire et le renvoie dans un en-tête
`Set-Cookie` (`Path=/`, `HttpOnly`). Aux visites suivantes il retrouve la
session depuis le cookie et incrémente son compteur. Un `sessionid` inconnu ou
falsifié reçoit une nouvelle session. Les sessions sont perdues à l'arrêt du
serveur.

```bash
curl -i -c jar -b jar http://localhost:8080/session
```

**Plusieurs types de CGI.** Une même location peut associer plusieurs extensions
à plusieurs interpréteurs, par exemple `.py` à `python3` et `.php` à `php-cgi`.

```bash
curl -i http://localhost:8080/cgi-bin/hello.py
curl -i http://localhost:8080/cgi-bin/hello.php
```

### 📝 Logger

Au démarrage, la sortie du serveur est redirigée vers `./webserv.log`, ce qui
garde le terminal propre. Chaque ligne est horodatée avec un niveau (`info`,
`error`). Sont enregistrés le démarrage, les adresses d'écoute, les connexions
acceptées et fermées, les lancements de CGI, l'arrêt, et les échecs d'appels
système comme `poll` ou `accept`. Si le fichier ne peut pas être ouvert, les logs
repartent vers le terminal.

```
11/09/2026 08:42:21 [info] accept 127.0.0.1:55348 on fd 7
11/09/2026 08:42:21 [info] GET /cgi-bin/cookie.py -> cgi started
11/09/2026 08:42:21 [info] close 127.0.0.1 on fd 7
```

À suivre pendant que le serveur tourne :

```bash
tail -f webserv.log
```

### 🔬 Tests

Les scripts shell et Python sont dans `tests/` : parsing de la conf, sockets
d'écoute, autoindex, DELETE, sortie et cycle de vie des CGI, fuites de
descripteurs, et un fuzzer de requêtes. Le tester de 42 s'utilise avec
`conf/tester.conf`.

```bash
./tests/test_conf.sh
./tests/test_autoindex.sh
python3 tests/fuzz_requests.py
```

Les descripteurs de fichiers sont vérifiés avec valgrind, qui détecte à la fois
les fuites et les doubles fermetures, en particulier dans le fils CGI :

```bash
valgrind --track-fds=yes --trace-children=no ./webserv conf/default.conf
```

### 🧠 À propos de curl

`curl` est un client HTTP en ligne de commande. Il envoie une requête à l'URL
donnée et affiche la réponse, ce qui est pratique pour tester le serveur sans
navigateur. Options utilisées dans ce README :

- `-i` : affiche aussi la ligne de statut et les en-têtes de réponse, pas
  seulement le corps.
- `-X METHODE` : choisit la méthode de la requête (`GET` par défaut).
- `--data-binary @fichier` : envoie le contenu du fichier comme corps de
  requête, tel quel. Transforme aussi la requête en `POST`.
- `-c jar` : enregistre les cookies posés par le serveur (`Set-Cookie`) dans un
  fichier nommé `jar`. Le nom est libre, jar signifie bocal à cookies.
- `-b jar` : lit les cookies stockés dans `jar` et les envoie dans un en-tête
  `Cookie`.
- `-v` : mode verbeux, affiche aussi les en-têtes envoyés par curl.

Ensemble, `-c jar -b jar` font se comporter curl comme un navigateur : la
première requête n'a pas de cookie et en reçoit un, les suivantes le renvoient.
Un `cat jar` montre le `sessionid` stocké.

### 📎 Ressources

- RFC 9110, HTTP Semantics
- RFC 9112, HTTP/1.1
- RFC 3875, The Common Gateway Interface
- RFC 6265, HTTP State Management (cookies)
- Documentation NGINX, nginx.org/en/docs
- Beej Guide to Network Programming
- MDN Web Docs, section HTTP
- Pages de manuel Linux : `socket`, `poll`, `fork`, `execve`, `pipe`

---

*Webserv, 42 Common Core.*
