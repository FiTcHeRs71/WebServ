web_serv/
├── Makefile
├── inc/                      # headers d'interface figés (S0-02) — corps vides pour l'instant
│   ├── ServerConfig.hpp
│   ├── Request.hpp
│   ├── Response.hpp
│   └── Connection.hpp
├── src/
│   ├── main.cpp
│   ├── config/                # Module A — ConfigParser, Resolver
│   ├── network/                # Module B — ListenSocket, ClientManager, Connection
│   ├── http/                   # Module C — RequestParser, Router, Handlers, ResponseBuilder
│   └── cgi/                    # Module D — CgiProcess
├── conf/
│   ├── default.conf
│   ├── tester.conf            # déjà présent
│   └── bad/                   # 20 confs invalides (S0-03)
├── www/                        # site statique de démo (T-04)
├── tests/
│   └── run.py                  # suite Python 30+ cas (T-01)
├── testeur/                    # tester officiel 42 (déjà présent)
├── YoupiBanane/                # arbo imposée par le tester (déjà présente)
└── docs/                       # déjà présent