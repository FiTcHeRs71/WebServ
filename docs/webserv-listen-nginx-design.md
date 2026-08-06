# Design — directive `listen` compatible NGINX (A-07)

Date : 2026-08-05
Module : config
Référence : https://nginx.org/en/docs/http/ngx_http_core_module.html#listen
Sujet : `web_serv.subject.pdf` p.6 (fonctions autorisées), p.9 (vhost hors scope), p.10 (interface:port)

## 1. Objectif

Aligner le parsing de `listen` sur NGINX pour les formes d'adresse et le
paramètre `default_server`, et introduire la table d'adresses qui servira de
contrat entre la config et la couche socket (B-01).

Hors périmètre : IPv6, sockets UNIX, `ssl`, `http2`, `quic`, `proxy_protocol`
et toutes les options socket (`backlog=`, `rcvbuf=`, `reuseport`, …). Ces
formes sont reconnues puis **rejetées avec un message nommé**, pas ignorées
silencieusement.

## 2. État actuel

`parse_listen()` (`srcs/config/utils.cpp:93`) :

- accepte `host:port` ou `port` seul, host implicite `0.0.0.0` ;
- impose une IPv4 stricte, donc rejette `*:8080`, `localhost:8080` et
  interprète `listen 127.0.0.1;` comme un numéro de port ;
- ne connaît aucun paramètre.

`ServerConfig::_Listens` est un `vector<pair<string, int> >` : le type ne peut
pas porter `default_server`.

`ConfigParser::check_listen()` (`srcs/config/config.cpp:344`) déduit le serveur
par défaut de l'absence de `server_name` et lève une exception sur conflit.
NGINX élit le premier serveur déclaré du couple `addr:port`.

## 3. Grammaire supportée

```
listen address[:port] [default_server];
listen port [default_server];
```

Désambiguïsation NGINX : un token **entièrement numérique** est un port, sinon
c'est une adresse.

| Écrit dans le `.conf`   | Résultat            | Note                                       |
| ----------------------- | ------------------- | ------------------------------------------ |
| `8080`                  | `0.0.0.0:8080`      | port seul, toutes interfaces               |
| `127.0.0.1:8080`        | `127.0.0.1:8080`    |                                            |
| `127.0.0.1`             | `127.0.0.1:80`      | adresse seule, port implicite              |
| `*:8080`                | `0.0.0.0:8080`      | `*` normalisé dès le parsing               |
| `*`                     | `0.0.0.0:80`        |                                            |
| `localhost:8080`        | `127.0.0.1:8080`    | via `getaddrinfo`                          |
| `[::]:8080`             | erreur              | `IPv6 is not supported by webserv`         |
| `unix:/tmp/s.sock`      | erreur              | `unix sockets are not supported by webserv`|
| `8080 ssl`              | erreur              | `listen parameter "ssl" is not supported`  |
| `8080 default_server`   | `0.0.0.0:8080` défaut |                                          |

Port valide : 1-65535. Un hostname résolvant vers plusieurs IPv4 produit
**un `ListenConfig` par adresse**, comme NGINX.

## 4. Structures

```
includes/ListenConfig.hpp                        (nouveau)

    struct ListenConfig
        string  Host;              // toujours une IPv4 résolue : "0.0.0.0", "127.0.0.1"
        int     Port;              // 1-65535
        bool    IsDefaultServer;   // paramètre default_server présent

includes/Config.hpp                              (ajout)

    struct AddrPortGroup
        string          Host;
        int             Port;
        vector<size_t>  ServerIndexes;   // index dans _Servers, ordre de déclaration
        size_t          DefaultIndex;    // index du serveur par défaut du groupe

    class ConfigParser
        vector<AddrPortGroup>  _AddrPorts;       // construit en passe 5
        void                   build_addr_port_groups(void);   // remplace check_listen()

includes/ServerConfig.hpp                        (modif)

    vector<ListenConfig>  _Listens;   // remplace vector<pair<string, int> >

srcs/config/listen.cpp                           (nouveau)

    vector<ListenConfig>  parse_listen(const vector<string> &tokens);
    static bool           split_addr_port(const string &token, string &addr, string &port);
    static bool           resolve_host(const string &host, vector<string> &out);
    static void           parse_parameters(const vector<string> &tokens, ListenConfig &cfg);
```

`*` est normalisé en `0.0.0.0` au parsing : aucun flag wildcard n'est conservé,
un `ListenConfig` porte toujours une IPv4 littérale.

`parse_listen()` et `is_valid_ipv4()` quittent `srcs/config/utils.cpp`, qui
dépasse sa responsabilité de fourre-tout.

## 5. Pipeline

```
passe 3   parsing des directives
            key "listen" -> parse_listen(tokens) -> 1..N ListenConfig

passe 4   valeurs par défaut
            si _Listens vide -> ListenConfig{ "0.0.0.0", DEFAULT_PORT, false }

passe 5   build_addr_port_groups()          remplace check_listen()
            regroupe par (Host, Port) exact
            élit le serveur par défaut de chaque groupe
            détecte les conflits
```

Règles de groupe :

- `0.0.0.0:8080` et `127.0.0.1:8080` forment **deux groupes distincts**, donc
  deux sockets. Une connexion sur `127.0.0.1` est servie par le groupe
  spécifique, les autres interfaces par le groupe wildcard. Comportement NGINX.
- Deux `default_server` dans un même groupe → erreur
  `a duplicate default server for 0.0.0.0:8080`.
- Aucun `default_server` explicite dans un groupe → le **premier serveur
  déclaré** du groupe. Remplace la règle actuelle « celui sans `server_name` ».
- Même `host:port` répété dans un seul bloc `server` → erreur
  `duplicate listen 0.0.0.0:8080`.
- `server_name` dupliqué dans un même groupe → **warning sur stderr**
  (`conflicting server name "a" on 0.0.0.0:8080, ignored`), le premier serveur
  déclaré gagne. Aligné sur NGINX ; l'ancien `throw` disparaît.

`ListenSockets` (aujourd'hui un squelette vide) consommera
`vector<AddrPortGroup>` : **un groupe = un socket**.

## 6. Décisions et contraintes

**Port par défaut = 80 partout.** `DEFAULT_PORT` passe de 8080 à 80, et le port
implicite d'une adresse sans port vaut 80. NGINX utilise `*:80` avec les droits
super-utilisateur et `*:8000` sinon, mais `geteuid()` n'est pas dans la liste
des fonctions autorisées (sujet p.6) : la bascule ne peut pas être reproduite.
Conséquence assumée : un bloc `server` sans `listen` échoue au `bind()` en
non-root. Les `.conf` du dépôt déclarent tous un port explicite et ne sont pas
affectés.

**C++98 strict** (`-Wall -Wextra -Werror`) : struct + fonctions libres, pas de
`std::function`, pas de lambda, pas de `stoi`. Conversion de port par `strtol`
comme le fait déjà `parse_body_size()`.

**`getaddrinfo` / `freeaddrinfo` / `gai_strerror`** sont explicitement
autorisés par le sujet (p.6). La résolution est faite **une seule fois, au
parsing** — jamais pendant le service.

**Le virtual host est hors scope du sujet** (p.9) mais autorisé. `server_name`
et `default_server` restent donc un chantier bonus : la table
`_AddrPorts` doit rester utilisable même si le routage par `Host:` n'est
jamais implémenté.

## 7. Découpage en étapes

1. **Squelettes.** `includes/ListenConfig.hpp`, `struct AddrPortGroup`,
   prototypes vides dans `srcs/config/listen.cpp`, migration de `_Listens` vers
   le nouveau type. Compile, comportement inchangé.
2. **Formes d'adresse.** `parse_listen` : token numérique vs adresse, `*`,
   adresse seule, `getaddrinfo`, rejets nommés IPv6 / unix.
3. **Paramètres.** `default_server` reconnu, tout autre paramètre rejeté par son
   nom.
4. **Table d'adresses.** `build_addr_port_groups()` : regroupement, élection du
   défaut, conflits ; retrait de `check_listen()`.
5. **Tests.** Nouveaux `conf/bad/`, `.conf` valides couvrant chaque forme, cas
   ajoutés à `tests/test_conf.sh`.

## 8. Tests

Fichiers d'erreur à ajouter dans `conf/bad/` :

| Fichier                              | Cas                                   | Étape |
| ------------------------------------ | ------------------------------------- | ----- |
| `22_listen_ipv6.conf`                | `listen [::]:8080;`                   | 2 ✅  |
| `23_listen_unix_socket.conf`         | `listen unix:/tmp/webserv.sock;`      | 2 ✅  |
| `24_listen_missing_addr.conf`        | `listen :8080;`                       | 2 ✅  |
| `25_listen_missing_port.conf`        | `listen 127.0.0.1:;`                  | 2 ✅  |
| `26_listen_multiple_colon.conf`      | `listen 1:2:3;`                       | 2 ✅  |
| `27_listen_unsupported_param.conf`   | `listen 8080 ssl;`                    | 3 ✅  |
| `28_listen_unknown_param.conf`       | `listen 8080 nawak;`                  | 3 ✅  |
| `29_listen_duplicate_param.conf`     | `default_server` deux fois            | 3 ✅  |
| `30_listen_duplicate_default.conf`   | deux `default_server` sur `*:8080`    | 4     |
| `31_listen_duplicate_in_server.conf` | `listen 8080;` deux fois dans un bloc | 4     |

Les cas 27 et 28 sont un couple : ils vérifient que les deux branches du
diagnostic rendent des messages distincts — « existe en NGINX, hors périmètre »
contre « n'existe nulle part ».

Le cas « IPv4 invalide » (`listen 500.0.0.0:8080;`) est déjà couvert par
`conf/bad/10_invalid_ip.conf`, qui passe désormais par `resolve_host()`.

`conf/bad/20_server_name_conflict.conf` devient un cas **valide** (warning) et
migre vers `conf/`. `tests/test_conf.sh` est adapté en conséquence.

`conf/default.conf` exerce chaque forme acceptée du tableau du §3 :
`0.0.0.0:8080`, `8081`, `*:9090`, `localhost:7070`, `127.0.0.1`, `*`.

Attention : les deux dernières prennent le port implicite 80, donc un port
privilégié. Le parsing les accepte, mais leur `bind()` échouera en non-root une
fois B-01 écrit. Si ça gêne, les déplacer vers un `conf/listen_forms.conf`
dédié et laisser `conf/default.conf` sur des ports hauts.

Comparaison contre NGINX (ticket T-03) pour les cas ambigus :
`listen 127.0.0.1;`, `*:8080` face à `127.0.0.1:8080`, `default_server`
implicite.
