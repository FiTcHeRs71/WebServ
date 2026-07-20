# Webserv — Mode d'emploi de l'équipe (à 3)

> Le « comment on bosse ensemble ». À lire avant d'écrire une ligne, à rouvrir à chaque doute.
> Le détail des tickets est dans [`webserv-backlog.md`](webserv-backlog.md). L'archi visuelle dans [`webserv-diagrammes.md`](webserv-diagrammes.md).

---

## 1. La méthode en 5 règles

1. **On découpe le long des frontières faibles, pas par fichiers.**
   4 modules **obligatoires** + 1 module **bonus**, 1 responsable chacun :

   ```
   Config ──(structs)──► Réseau ──(octets)──► HTTP        + CGI (à cheval B/C)
     A(dev1)              B(dev2)              C(dev3)         D(à 2, plus tard)
                                                              + Bonus E (après le mandatory à 100%)
   ```

   - **A – Config** : lit le `.conf`, produit des structs. Ignore sockets et HTTP.
   - **B – Réseau** : `poll()`, sockets, buffers, timeouts. Transporte des octets, ne connaît rien au HTTP.
   - **C – HTTP** : parse la requête, route, construit la réponse. Ne touche jamais un socket.
   - **D – CGI** : `fork`+`execve`+pipes non-bloquants dans le même `poll`. **⚠️ OBLIGATOIRE** (au moins 1 CGI, sujet p.13) — démarré tard, validé à 2.
   - **E – Bonus** (facultatif) : cookies + sessions, et plusieurs types de CGI. **Ne se code QUE si tout l'obligatoire est parfait** — le sujet n'évalue le bonus que si le mandatory est 100 % OK.

2. **On fige les interfaces AVANT de coder** (le point qui fait ou défait le projet à plusieurs).
   Tant que ces signatures ne bougent pas, les 3 codent en parallèle sans se marcher dessus.
   En changer une = accord des 3.

   | Frontière | Contrat |
   |---|---|
   | A → B | `const std::vector<ServerConfig>&` |
   | A → C | `const LocationConfig* resolve(host, port, uri)` |
   | B → C | `RequestParser::feed(data, n)` → `INCOMPLETE / COMPLETE / ERROR` |
   | C → B | `bool Response::serialize(std::string& out)` |
   | C → D | `CgiProcess::start(req, loc)` → expose `read_fd` / `write_fd` que B ajoute au poll |

3. **Tickets à DoD binaire.** Chaque ticket a une *Definition of Done* vérifiable objectivement,
   pas « ça marche ». La DoD teste la vraie difficulté (voir §2).

4. **Chemin critique + merges de synchro.** On intègre en continu, jamais à la fin (voir §3).

5. **Relecture croisée obligatoire.** Rien ne passe en `Done` sans qu'un autre l'ait relu.
   À la soutenance, chacun peut être interrogé sur n'importe quelle ligne, même celles des autres.

---

## 2. Le système de tickets

### Format d'un ticket

> `[ID] titre — dépend de — Definition of Done`

Le champ qui compte, c'est la **DoD** : une condition **binaire**, qui teste la vraie difficulté.

- ❌ « Parser les headers » → on croit que c'est fini alors que non.
- ✅ `C-01` : *« Envoi octet par octet en telnet → même résultat qu'en un bloc »* → teste le parsing incrémental.

### Outil : GitHub Projects (gratuit, collé au repo)

- **Colonnes** : `Backlog` → `In progress` → `Needs review` → `Done`
- **Labels** : `mod:config` · `mod:net` · `mod:http` · `mod:cgi` · `blocked` · `needs-review`
- Les IDs du backlog (`B-03`, `C-06`…) servent de titres → import en ~20 min.
- Dans les commits : `Closes #12` ferme le ticket automatiquement au merge.

### Workflow d'un ticket (le cycle de vie)

1. Je prends un ticket `Backlog` → je me l'assigne → `In progress`.
2. Je crée une branche : `feat/<mod>-<id>` (ex. `feat/b-03-connection-buffers`).
3. Je code **contre les interfaces figées**, pas contre le vrai code des autres.
4. DoD atteinte + testée → je pousse → j'ouvre une PR → colonne `Needs review`, label `needs-review`.
5. Un autre équipier relit, teste la DoD lui-même, approuve.
6. Merge → `Closes #NN` → colonne `Done`. **Jamais de merge sans relecture.**

### Règle des dépendances

Un ticket avec une dépendance non satisfaite ne se prend pas → label `blocked`.
A, B et C démarrent en parallèle après le Sprint 0. **D (CGI) attend que C soit avancé.**

---

## 3. Ordre de déblocage & points de synchro

```
Sprint 0 (interfaces gelées)  ── tout le monde
        │
        ├── A : A-01→A-04 ─────► A-05 A-06 ──► aide D + démo
        ├── B : B-01→B-04 ─────► B-05 B-06 ──► B-07 + stress test
        └── C : C-01→C-05 ─────► C-06→C-11 ──► (libère qqn pour D)
                                        │
                                        └── D : D-01→D-05 (à 2)
```

**Les 4 points de synchro = merges obligatoires sur `main` :**

1. **Fin Sprint 0** — headers gelés. Toute modif ultérieure = accord des 3.
2. **B-03 + C-05** — premier « hello world » bout-en-bout dans le navigateur. **C'est le vrai début du projet.**
3. **C-06** — site statique servi (CSS/JS/images). Deuxième merge.
4. **D-05** — feature complete. Reste tests + README.

À chaque point de synchro : tout le monde merge, on vérifie que ça s'assemble. On n'attend jamais la fin pour intégrer.

---

## 4. Comment tester un module ISOLÉMENT

Le but : chaque dev valide son module **sans attendre** que les 2 autres soient prêts.
La clé, c'est que les interfaces sont figées → on remplace le voisin par un bouchon (« mock »).

### A – Config (aucune dépendance, le plus simple à tester seul)

Un `main` de test qui parse un `.conf` et **dump** les structs obtenues.

```cpp
// tests/test_config.cpp
int main(int ac, char** av) {
    std::vector<ServerConfig> cfg = ConfigParser::parse(av[1]);
    ConfigParser::debugPrint(cfg);   // affiche listen / root / locations…
}
```

- Confs **valides** → le dump doit correspondre à ce qu'on attend.
- 10 confs **invalides** dans `conf/bad/` → chacune doit sortir un message clair + `exit(1)`, **aucune acceptée**.
- `resolve()` → tests unitaires du match de préfixe : `/kapouet/pouic/toto/pouet` → `/tmp/www/pouic/toto/pouet`.

### B – Réseau (sans le vrai HTTP : on renvoie une réponse hardcodée)

B se teste avec des **outils réseau bruts**, en bouchonnant C par une réponse en dur.

```cpp
// à la place du vrai RequestParser/Response :
std::string out = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nhi\n";
```

- **Connexion** : `telnet localhost 8080`, plusieurs ports en parallèle.
- **Send partiel** : renvoyer un gros body → vérifier qu'il part en plusieurs `send` (POLLOUT).
- **Déconnexions** : `Ctrl-C` du client pendant l'envoi → pas de crash.
- **Zéro fd leak** : `ls /proc/<pid>/fd | wc -l` stable avant/après une rafale de connexions.
- **Timeout** : ouvrir une connexion et ne rien envoyer → doit se fermer (408), jamais de hang.
- ⚠️ **La règle qui donne le 0** : aucun `read`/`recv`/`send` sans que `poll` ait signalé le fd prêt, et **zéro lecture d'`errno`** après. À vérifier à la relecture.

### C – HTTP (sans le vrai réseau : on `feed()` depuis une string)

C ne dépend pas d'un socket. On lui donne des octets directement.

```cpp
// tests/test_http.cpp
RequestParser p;
std::string raw = "GET /index.html HTTP/1.1\r\nHost: x\r\n\r\n";
for (size_t i = 0; i < raw.size(); ++i)      // octet par octet !
    p.feed(&raw[i], 1);
assert(p.state() == COMPLETE);
```

- **Parsing incrémental** : envoyer octet par octet doit donner le même résultat qu'en un bloc.
- **Fuzz** : requêtes tronquées, headers géants, méthode bidon → `400/501/505`, **jamais de crash**.
- **Body** : `Content-Length` 1 octet au-dessus de `client_max_body_size` → `413` ; en dessous → OK.
- **Chunked** : `curl -H "Transfer-Encoding: chunked" --data-binary @big`.
- Une fois B mergé : test navigateur réel + comparaison **nginx** (`curl -I` sur `/dir` sans slash, codes, headers).

### D – CGI (à 2) — OBLIGATOIRE

- `hello.py` renvoie du texte → `D-01` OK.
- Script qui **dump son env** → comparé à la spec des meta-variables (`D-02`).
- `while True: pass` → doit finir en `504` + `ps` propre (zéro zombie, `D-05`).

### E – Bonus (uniquement si le mandatory est 100 % OK)

- **Cookies** (`E-01`) : une route pose un `Set-Cookie`, le navigateur (ou `curl -c/-b`) le renvoie au tour suivant.
- **Sessions** (`E-02`) : 2 clients → 2 sessions distinctes ; exemple simple (compteur de visites ou mini-login) démontrable.
- **Plusieurs CGI** (`E-03`) : un `.php` ET un `.py` servis par le même serveur selon la config.

### Transverse — la vraie suite de tests

Un script Python (`requests` + `socket` brut) qui rejoue **30+ cas** dont des requêtes malformées et des envois lents :

```
python3 tests/run.py     # doit être tout vert avant chaque merge de synchro
```

Plus, pour B : stress test `siege -b` / `ab` → availability 100 %, RAM stable, `valgrind` propre.

### Le tester officiel 42 (`tester/`)

Les binaires du sujet sont dans `tester/` : `tester` (teste le serveur) et `cgi_tester` (sert de programme CGI que **votre** serveur exécute). **Passer ce test est le minimum avant l'éval** — mais il ne teste pas tout, gardez la suite Python à côté.

**Étape 1 — créer l'arborescence attendue** (contenu des fichiers libre) :

```
YoupiBanane/
├── youpi.bad_extension
├── youpi.bla
├── nop/
│   ├── youpi.bad_extension
│   └── other.pouic
└── Yeah/
    └── not_happy.bad_extension
```

**Étape 2 — une config qui répond EXACTEMENT à ça** (le tester l'impose mot pour mot) :

| Route | Comportement exigé |
|---|---|
| `/` | GET **uniquement** |
| `*.bla` (extension) | POST → exécute le CGI `cgi_tester` |
| `/post_body` | POST, `client_max_body_size` = **100** |
| `/directory/` | GET, `root` = `YoupiBanane`, cherche `youpi.bad_extension` si aucun fichier demandé |

**Étape 3 — lancer** (serveur démarré au préalable, sur le bon port) :

```
./webserv conf/tester.conf &
./tester/tester http://localhost:8080
```

> C'est un bon jalon de synchro : dès que le point n°3 (site statique servi) est atteint, on doit viser à faire passer ce tester. Le CGI (`*.bla` + `cgi_tester`) ne passera qu'après le module D.

---

## 5. Checklist soutenance (à garder sous le coude)

- [ ] `-Wall -Wextra -Werror -std=c++98`, aucun relink inutile.
- [ ] Un seul `poll()` (ou équivalent) pour **tout** : listen, clients, pipes CGI.
- [ ] Jamais d'I/O sans passage par `poll`, jamais d'`errno` lu après une I/O.
- [ ] Aucune fuite mémoire ni fd (`valgrind`, compteur de fd stable).
- [ ] README en anglais : 1re ligne en italique avec les 3 logins + usage de l'IA.
- [ ] Chacun sait expliquer les **3** modules, pas seulement le sien.
- [ ] Chaque feature du sujet démontrable en **une commande**.
- [ ] Le tester officiel (`tester/tester`) passe (voir §4).


## 6. Notes

Le flux complet, concret

1. Je prends l'issue #23 → je m'assigne → colonne "In progress"
2. git checkout -b feat/b-03-connection-buffers
3. ... code + commits ...
4. git push -u origin feat/b-03-connection-buffers
5. J'ouvre la PR. Dans la DESCRIPTION j'écris :
        Implémente les buffers de connexion.
        Closes #23
6. Un équipier relit, teste la DoD, approuve
7. Il MERGE la PR  →  #23 se ferme automatiquement
                   →  (si workflow activé) la carte passe en "Done"