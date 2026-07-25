# Feuille de route — Parsing du fichier `.conf`

*Date : 2026-07-22 — Périmètre : transformer `conf/default.conf` en objets `ServerConfig` / `LocationConfig` exploitables, et rejeter proprement les 20 configs de `conf/bad/`.*

---

## 1. État actuel

| Élément | État |
|---|---|
| `srcs/config/config.cpp` | brouillon A-01 en cours (lexer + utils) — c'est ici que tout se construit |
| `includes/ServerConfig.hpp` | squelette (forme canonique seule, aucun champ) |
| `includes/LocationConfig.hpp` | squelette (forme canonique seule, aucun champ) |
| `conf/default.conf` | **la** référence de grammaire |
| `conf/bad/*.conf` | 20 cas d'erreur = la suite de tests |
| `conf/tester.conf` | **dans le périmètre** — réécrit sans regex : les `.bla` se routent par `cgi_ext` (match par extension) |

Contrainte de compilation : `-Wall -Wextra -Werror -std=c++98`.

---

## 2. Décisions actées

### 2.1 Pas de `std::map<std::string, std::string>` comme structure de stockage

L'idée de départ était de stocker chaque directive sous forme clé → valeur. Écartée, parce que la grammaire de `default.conf` la casse sur quatre points :

| Cas réel dans la conf | Ce que fait une `map<string,string>` |
|---|---|
| `error_page 404 …` répété 4 fois | une seule clé `"error_page"` → 3 valeurs écrasées |
| `allow_methods GET POST` | stocke `"GET POST"` → re-parsing de la string à chaque requête |
| `client_max_body_size 10M` | stocke `"10M"` → conversion repoussée à l'exécution |
| `location /` imbriqué dans `server` | la map est plate, elle n'exprime aucune imbrication |

Le point bloquant est le troisième : si les valeurs restent des chaînes brutes, les erreurs `08` (port hors borne), `09` (port non numérique), `11` (suffixe de taille invalide) et `12` (taille négative) ne sont détectées qu'au moment de servir une requête. Or elles doivent l'être **au démarrage** — le sujet impose qu'un crash vaille 0.

**La map reste pertinente en interne**, là où la clé est typée et la valeur unique :
- `std::map<int, std::string> _error_pages` → code HTTP vers page
- `std::map<std::string, std::string> _cgi` → extension vers interpréteur

### 2.2 Niveau de rapport d'erreur : message seul

Pas de numéro de ligne. Conséquence directe : **le lexer produit un `std::vector<std::string>`**, pas une `struct Token` — inutile de trimballer une position.

### 2.3 Locations par préfixe uniquement, pas de regex

`docs/webserv-fichier-conf.md` fait foi : « no regex required, le CGI se déclenche par extension ». `LocationConfig` n'a donc **pas** de champ « type de match » — un `std::string _path` suffit. Le CGI se déclenche en comparant l'extension du fichier demandé à `cgi_ext`.

### 2.4 Architecture retenue : lexer + parseur descendant

Alternative écartée : lecture ligne par ligne avec une machine à états (`OUTSIDE` / `IN_SERVER` / `IN_LOCATION`). Plus rapide à démarrer, mais le format devient prisonnier de la mise en page — et `default.conf` écrit déjà `server` et `{` sur deux lignes séparées. Chaque cas tordu ajoute un `if`, et au bout de 20 fichiers `bad/` c'est ingérable.

---

## 3. Le flux

Trois passes strictement séquentielles, une responsabilité chacune :

```
conf/default.conf
       │
       ▼  PASSE 1 — lexer      : texte → std::vector<std::string>
   ["server", "{", "listen", "0.0.0.0:8080", ";", …, "}"]
       │
       ▼  PASSE 2 — parseur    : tokens → objets typés (index qui avance dans le vector)
   std::vector<ServerConfig>  ←  chacun contient  std::vector<LocationConfig>
       │
       ▼  PASSE 3 — validation : cohérence entre blocs
   config prête, ou exception → message + exit(1)
```

### Découpage en fichiers

Aucun nouveau `.cpp` — on remplit ceux que le `Makefile` connaît déjà.

| Fichier | Rôle |
|---|---|
| `srcs/config/config.cpp` | lexer + parseur + validation croisée — la **machinerie** |
| `srcs/config/ServerConfig.cpp` | classe de **données** : champs, getters, `Resolve()` |
| `srcs/config/LocationConfig.cpp` | idem au niveau location |
| `includes/config.hpp` | **à créer** — fonctions libres du parseur + type d'exception |

Principe structurant : **`ServerConfig` ne se parse pas elle-même.** Elle ignore les tokens, les `;` et les accolades ; elle stocke des valeurs déjà validées et sait répondre à `Resolve()`. Le parseur est un jeu de fonctions libres dans `config.cpp` qui construisent ces objets depuis l'extérieur. C'est ce qui rend les deux testables séparément et évite que `ServerConfig.cpp` gonfle en mélangeant parsing et logique métier.

---

## 4. PASSE 1 — Le lexer *(spécifié, prêt à coder)*

```cpp
std::vector<std::string> tokenize(const std::string &path);
```

**Invariant : le lexer ne juge jamais.** Il ignore ce qu'est un `server`, si une accolade est de trop ou si un `;` manque. Il découpe, point. Sa seule erreur possible est un fichier illisible ; toutes les erreurs de grammaire appartiennent au parseur.

### Algorithme

Lecture ligne par ligne (`std::getline`) :

1. **Tronquer la ligne au premier `#`** — les commentaires disparaissent avant tout le reste.
2. Parcourir les caractères restants avec un accumulateur `std::string word` :
   - **espace / tab / `\r`** → si `word` non vide, la pousser et la vider ;
   - **`;` `{` `}`** → pousser `word` si non vide, **puis** pousser le caractère comme token à part entière ;
   - **tout le reste** → l'ajouter à `word`.
3. En fin de ligne, pousser `word` si elle n'est pas vide.

Le point 2 est le cœur : `8080;` collé donne bien `"8080"` puis `";"`, et `server{` donne `"server"` puis `"{"`. La mise en page devient totalement indifférente.

### Sortie attendue sur `default.conf`

```
"location" "/" "{" "allow_methods" "GET" ";" "root" "./www" ";"
"index" "index.html" ";" "}"
```

### Hors périmètre (YAGNI)

Guillemets (`root "./mon dossier"`) et caractères d'échappement : la grammaire n'en a aucun besoin, les ajouter doublerait la fonction.

### Cas limite

`06_empty_file.conf` produit un vector **vide**, renvoyé sans erreur. C'est le parseur qui dira « aucun bloc `server` trouvé ».

### Vérification avant de passer à la suite

Écrire un `main` de debug qui affiche le vector token par token, et le passer sur les 21 fichiers de conf. Le lexer doit être validé **avant** d'écrire une ligne de parseur.

---

## 5. PASSE 2 — Le parseur *(à détailler)*

Descente récursive : un `size_t i` avance dans le vector de tokens.

```
parseFile()      → boucle : attend "server", puis parseServer()
parseServer()    → attend "{", boucle sur les directives,
                   "location" → parseLocation(), attend "}"
parseLocation()  → attend un chemin, "{", les directives, "}"
```

Chaque directive est convertie **immédiatement** dans son type définitif :

| Directive | Type cible |
|---|---|
| `listen 0.0.0.0:8080` | `std::string _host` + `int _port` |
| `client_max_body_size 10M` | `size_t` = `10485760` |
| `allow_methods GET POST` | `std::set<std::string>` |
| `error_page 404 /errors/404.html` | entrée dans `std::map<int, std::string>` |
| `autoindex on` | `bool` |

Si une conversion échoue → exception immédiate.

**Reste à définir :** la liste exacte des champs de `ServerConfig` et `LocationConfig`, le mécanisme de détection des doublons (erreurs `16` et `18`), et l'héritage `server` → `location` pour `client_max_body_size`.

---

## 6. PASSE 3 — Validation croisée *(à détailler)*

Ce qui ne se voit pas depuis un seul bloc, une fois tous les `ServerConfig` construits :

- **`17`** — deux blocs `server` qui déclarent le même couple `ip:port`
- **`19`** — une location avec `cgi_ext` mais sans `cgi_pass`

---

## 7. Taxonomie des 20 erreurs — quelle passe attrape quoi

| # | Fichier | Passe |
|---|---|---|
| 01 | `brace_not_closed` | 2 — structure |
| 02 | `brace_extra` | 2 — structure |
| 03 | `missing_semicolon` | 2 — structure |
| 04 | `location_outside_server` | 2 — structure |
| 05 | `unknown_directive` | 2 — structure |
| 06 | `empty_file` | 2 — structure (vector vide) |
| 07 | `directive_no_value` | 2 — structure |
| 08 | `port_out_of_range` | 2 — conversion |
| 09 | `port_not_numeric` | 2 — conversion |
| 10 | `invalid_ip` | 2 — conversion |
| 11 | `body_size_bad_suffix` | 2 — conversion |
| 12 | `body_size_negative` | 2 — conversion |
| 13 | `error_page_invalid_code` | 2 — conversion |
| 14 | `allow_methods_unknown` | 2 — conversion |
| 15 | `autoindex_invalid_value` | 2 — conversion |
| 16 | `listen_duplicated` | 2 — doublon |
| 17 | `server_listen_conflict` | **3** — croisée |
| 18 | `root_duplicated` | 2 — doublon |
| 19 | `cgi_ext_without_pass` | **3** — croisée |
| 20 | `return_malformed` | 2 — structure (nombre d'arguments) |

Aucun cas n'échoit au lexer — cohérent avec l'invariant de la section 4.

---

## 8. Prochaine étape

Reprendre le design aux sections **5** et **6** : structures de données exactes, détection des doublons, héritage `server` → `location`, et forme de l'exception. Puis coder la passe 1 et la valider sur les 21 fichiers avant d'attaquer la passe 2.
