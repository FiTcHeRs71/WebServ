# Arborescence du projet

*Reflète l'état réel du repo. Les modules (A/B/C/D) et tickets renvoient à [`webserv-backlog.md`](webserv-backlog.md).*

```
webserv/
├── Makefile                       # all / clean / fclean / re — -Wall -Wextra -Werror -std=c++98
├── update_makefile.sh             # régénère la liste des sources du Makefile
├── web_serv.subject.pdf           # sujet 42
├── .gitignore
│
├── includes/                      # headers d'interface figés (S0-02)
│   ├── ServerConfig.hpp           # Module A — données bloc server
│   ├── LocationConfig.hpp         # Module A — données bloc location
│   ├── ListenSockets.hpp          # Module B — sockets d'écoute
│   ├── Connection.hpp             # Module B — buffers in/out d'un client
│   ├── Request.hpp                # Module C — requête parsée
│   ├── Response.hpp               # Module C — réponse à sérialiser
│   └── CgiProcess.hpp             # Module D — process CGI (fork/execve/pipes)
│                                  # (config.hpp — à créer : parseur + exception, cf. feuille de route)
│
├── srcs/
│   ├── main.cpp
│   ├── config/                    # Module A — Config (dev 1)
│   │   ├── config.cpp             #   lexer + parseur + validation — VIDE pour l'instant (ticket A-01)
│   │   ├── ServerConfig.cpp
│   │   └── LocationConfig.cpp
│   ├── network/                   # Module B — Réseau (dev 2)
│   │   ├── ListenSockets.cpp
│   │   ├── Connection.cpp
│   │   └── network.cpp
│   ├── http/                      # Module C — HTTP (dev 3)
│   │   ├── Request.cpp
│   │   ├── Response.cpp
│   │   └── http.cpp
│   └── cgi/                       # Module D — CGI (à 2, après C)
│       ├── CgiProcess.cpp
│       └── cgi.cpp
│
├── conf/
│   ├── default.conf               # conf de référence (grammaire cible)
│   ├── tester.conf                # conf attendue par le tester officiel 42
│   └── bad/                       # 20 confs invalides = suite de tests du parseur (S0-03)
│       ├── 01_brace_not_closed.conf
│       ├── 02_brace_extra.conf
│       ├── ...                    # cf. taxonomie des 20 erreurs dans la feuille de route
│       └── 20_return_malformed.conf
│
├── www/                           # site statique de démo (T-04)
│   ├── index.html
│   └── errors/
│       ├── 404.html
│       └── 50x.html
│
├── YoupiBanane/                   # arbo imposée par le tester officiel 42
│   ├── youpi.bad_extension
│   ├── youpi.bla
│   ├── nop/
│   │   ├── youpi.bad_extension
│   │   └── other.pouic
│   └── Yeah/
│       └── not_happy.bad_extension
│
├── tests/
│   ├── run.py                     # suite Python 30+ cas (T-01)
│   └── testeur/                   # tester officiel 42
│       ├── tester                 #   teste notre serveur
│       └── cgi_tester             #   CGI que NOTRE serveur exécute
│
└── docs/                          # documentation d'équipe (backlog, mode d'emploi, diagrammes…)
```
