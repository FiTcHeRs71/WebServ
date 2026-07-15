# 05 — Configuration et routing

## 1. Ce que le sujet exige

Ta config doit permettre de :
- définir toutes les paires `interface:port` ;
- définir des pages d'erreur ;
- limiter la taille du body client ;
- et par route (sans regex) : méthodes autorisées, redirection HTTP, root, autoindex on/off, fichier par défaut, upload autorisé + destination, exécution CGI par extension.

Le sujet dit « inspire-toi du bloc `server` de nginx ». Suis ce conseil : le correcteur connaît nginx, et une config qui lui ressemble se lit toute seule.

## 2. Une config cible

```nginx
server {
    listen              0.0.0.0:8080;
    server_name         localhost webserv.local;
    client_max_body_size 10M;

    error_page 404      /errors/404.html;
    error_page 500 502 503 504  /errors/50x.html;

    location / {
        root            ./www/site1;
        index           index.html index.htm;
        allow_methods   GET;
        autoindex       off;
    }

    location /uploads {
        root            ./www/site1;
        allow_methods   GET POST DELETE;
        autoindex       on;
        upload_store    ./www/site1/uploads;
    }

    location /cgi-bin {
        root            ./www/site1;
        allow_methods   GET POST;
        cgi_ext         .py /usr/bin/python3;
        cgi_ext         .php /usr/bin/php-cgi;
    }

    location /old {
        return          301 /new;
    }

    location /kapouet {
        root            /tmp/www;
        allow_methods   GET;
        autoindex       on;
    }
}

server {
    listen              0.0.0.0:8081;
    client_max_body_size 1M;

    location / {
        root            ./www/site2;
        index           index.html;
        allow_methods   GET;
    }
}
```

Le `/kapouet` reprend l'exemple du sujet : `/kapouet/pouic/toto/pouet` doit chercher `/tmp/www/pouic/toto/pouet`. Mets-le dans ta config de démo, le correcteur va le tester **mot pour mot**.

## 3. La grammaire

```
config      := server*
server      := "server" "{" ( directive | location )* "}"
location    := "location" path "{" directive* "}"
directive   := name arg+ ";"
```

Trois niveaux de traitement :

**① Tokenizer.** Découpe en tokens : `{`, `}`, `;`, et les mots. Gère les commentaires `#` jusqu'à la fin de ligne, et les espaces/tabs/retours ligne comme séparateurs.

Piège : `location /uploads{` sans espace. Ton tokenizer doit traiter `{`, `}` et `;` comme des tokens indépendants, pas comme des caractères de mot. Sinon `/uploads{` devient un seul token.

**② Parser.** Consomme les tokens, construit les structs, vérifie la syntaxe (accolades appariées, `;` présents, directive connue, bon nombre d'arguments).

**③ Validateur.** Vérifie la sémantique : port dans [1,65535], root existant, méthode connue, code de redirection dans les 3xx, pas deux `location` avec le même path, taille bien formée.

Sépare bien les trois. Un parser qui valide au vol devient illisible et rate des cas.

## 4. Les structures

```cpp
struct LocationConfig {
    std::string                        path;          // "/uploads"
    std::string                        root;
    std::vector<std::string>           index;
    std::set<std::string>              allowMethods;  // {"GET","POST"}
    bool                               autoindex;
    std::string                        uploadStore;   // "" = upload interdit
    std::map<std::string, std::string> cgi;           // ".py" -> "/usr/bin/python3"
    int                                redirectCode;  // 0 = pas de redirection
    std::string                        redirectTarget;
    size_t                             clientMaxBodySize;  // hérité du server si absent
};

struct ServerConfig {
    std::string                   host;          // "0.0.0.0"
    int                           port;
    std::vector<std::string>      serverNames;
    size_t                        clientMaxBodySize;
    std::map<int, std::string>    errorPages;    // 404 -> "/errors/404.html"
    std::vector<LocationConfig>   locations;
};
```

`std::set` pour les méthodes : la question « autorisée ? » devient `loc.allowMethods.count(m)`, et les doublons disparaissent tout seuls.

`redirectCode == 0` plutôt qu'un `bool hasRedirect` : un champ au lieu de deux, et pas de risque d'incohérence.

## 5. `client_max_body_size`

nginx accepte `10M`, `1024k`, `512`. Ta fonction de parsing :

```cpp
bool parseSize(const std::string& s, size_t& out)
{
    if (s.empty()) return false;

    size_t i = 0;
    unsigned long long v = 0;
    while (i < s.size() && std::isdigit(s[i])) {
        v = v * 10 + (s[i] - '0');
        if (v > 0xFFFFFFFFULL) return false;   // overflow
        ++i;
    }
    if (i == 0) return false;                   // pas de chiffre

    unsigned long long mult = 1;
    if (i < s.size()) {
        char c = std::tolower(s[i]);
        if      (c == 'k') mult = 1024ULL;
        else if (c == 'm') mult = 1024ULL * 1024;
        else if (c == 'g') mult = 1024ULL * 1024 * 1024;
        else return false;
        ++i;
    }
    if (i != s.size()) return false;            // du texte après le suffixe

    v *= mult;
    if (v > 0xFFFFFFFFULL) return false;
    out = (size_t)v;
    return true;
}
```

Cas à tester : `10M` ✓, `0` ✓ (illimité chez nginx — décide et documente), `abc` ✗, `10MB` ✗, `-5` ✗, `10 M` ✗, `99999999999G` ✗ (overflow).

## 6. La résolution d'une location

**L'algorithme : le préfixe le plus long qui matche.**

```cpp
const LocationConfig* ServerConfig::resolve(const std::string& uri) const
{
    const LocationConfig* best = NULL;
    size_t bestLen = 0;

    for (size_t i = 0; i < locations.size(); ++i) {
        const std::string& p = locations[i].path;
        if (uri.compare(0, p.size(), p) != 0)
            continue;

        // frontière de segment : /upload ne doit pas matcher /uploadsecret
        if (p != "/" && uri.size() > p.size() && uri[p.size()] != '/')
            continue;

        if (p.size() > bestLen) { bestLen = p.size(); best = &locations[i]; }
    }
    return best;   // NULL si pas même de location "/"
}
```

**La frontière de segment est le piège.** Sans elle, `location /upload` matche l'URI `/uploadsecret/config` et tu sers un fichier depuis la mauvaise racine. C'est une faille, pas juste un bug.

Exemple avec `/`, `/uploads`, `/cgi-bin` :

| URI | Location choisie |
|---|---|
| `/` | `/` |
| `/index.html` | `/` |
| `/uploads` | `/uploads` |
| `/uploads/a.png` | `/uploads` |
| `/uploadsecret` | `/` ← grâce à la frontière |
| `/cgi-bin/x.py` | `/cgi-bin` |

**Choisir le serveur.** Avant la location, il faut le bon `ServerConfig`. Plusieurs blocs `server` peuvent partager un port :
1. filtre sur `host:port` ;
2. si plusieurs, matche `server_name` avec le header `Host` (en enlevant le `:port`) ;
3. aucun match → le **premier** bloc de ce port est le serveur par défaut. C'est ce que fait nginx.

Sans virtual hosts, l'étape 2 saute et tu prends toujours le premier. Mais implémente 1 et 3, c'est 10 lignes.

## 7. `root` : la transformation URI → chemin disque

**La règle nginx** (celle du sujet) : le chemin complet de l'URI est concaténé au root.

```
location /kapouet { root /tmp/www; }
URI = /kapouet/pouic/toto/pouet
  ->  /tmp/www/kapouet/pouic/toto/pouet
```

**Sauf que le sujet dit** : `/kapouet/pouic/toto/pouet` doit chercher `/tmp/www/pouic/toto/pouet`. Le préfixe `/kapouet` est **retiré**. C'est le comportement de `alias` chez nginx, pas de `root`.

Le sujet fait autorité. Implémente ce qu'il décrit :

```cpp
std::string buildPath(const LocationConfig& loc, const std::string& uri)
{
    std::string rest = uri.substr(loc.path.size());   // retire le préfixe
    if (!rest.empty() && rest[0] != '/') rest = "/" + rest;
    if (rest.empty()) rest = "/";
    return loc.root + rest;
}
```

Vérifie sur l'exemple exact du sujet, le correcteur le tapera. Et documente ton choix dans le README : « nos `location` se comportent comme `alias` chez nginx, conformément à l'exemple du sujet ». Si le correcteur pinaille, tu as la phrase du sujet à montrer.

## 8. `index` et le slash final

```cpp
// après buildPath, on stat()
struct stat st;
if (stat(path.c_str(), &st) != 0)
    return error(404);

if (S_ISDIR(st.st_mode)) {
    // 1. l'URI se termine-t-elle par '/' ?
    if (uri[uri.size()-1] != '/')
        return redirect(301, uri + "/");

    // 2. un fichier index existe-t-il ?
    for (size_t i = 0; i < loc.index.size(); ++i) {
        std::string cand = path + "/" + loc.index[i];
        if (stat(cand.c_str(), &st) == 0 && S_ISREG(st.st_mode))
            return serveFile(cand);
    }

    // 3. autoindex ?
    if (loc.autoindex)
        return serveAutoindex(path, uri);

    return error(403);
}
```

**Le 301 sur le dossier sans slash final** est ce que fait nginx, et il y a une raison : sur `/dir` (sans slash), le navigateur résout un lien relatif `img.png` en `/img.png`. Sur `/dir/`, il le résout en `/dir/img.png`. Sans la redirection, toutes les images de ton autoindex sont cassées. Le correcteur qui compare à nginx le verra.

Ordre non négociable : **slash d'abord, index ensuite, autoindex en dernier, 403 sinon**.

## 9. L'autoindex

Génère du HTML à la volée avec `opendir`/`readdir`/`closedir` :

```cpp
DIR* d = opendir(path.c_str());
if (!d) return error(403);

std::vector<std::string> entries;
struct dirent* e;
while ((e = readdir(d)) != NULL) {
    if (std::strcmp(e->d_name, ".") == 0) continue;
    entries.push_back(e->d_name);
}
closedir(d);
std::sort(entries.begin(), entries.end());
```

Points :
- Garde `..`, jette `.`. Le lien parent est attendu.
- **Échappe le HTML** dans les noms de fichiers. Un fichier nommé `<script>alert(1)</script>.txt` est créable sous Linux. Sans échappement, tu as un XSS stocké dans ton propre serveur. `&` `<` `>` `"` `'` → entités.
- **URL-encode les hrefs.** Un fichier `mon fichier.txt` donne un lien cassé sans `%20`.
- `stat()` chaque entrée pour la taille et la date, ajoute un `/` aux dossiers. C'est du confort, mais ça se voit.
- **Ferme toujours le DIR.** Un `return` au milieu sans `closedir` = fuite de fd.

## 10. Les pages d'erreur

```cpp
std::string Response::errorBody(int code, const ServerConfig& srv)
{
    std::map<int, std::string>::const_iterator it = srv.errorPages.find(code);
    if (it != srv.errorPages.end()) {
        std::string content;
        if (readFile(resolveErrorPath(it->second, srv), content))
            return content;
        // la page custom est introuvable -> on tombe sur la built-in,
        // surtout pas une récursion vers error(404)
    }
    return defaultErrorBody(code);
}
```

**Le piège de la récursion.** `error_page 404 /errors/404.html` et le fichier n'existe pas → tu appelles `error(404)` → qui cherche `/errors/404.html` → qui n'existe pas → boucle infinie → stack overflow → crash → **0**. Un correcteur malin renomme ta page d'erreur pendant la soutenance.

La parade : la fonction qui sert une page d'erreur ne passe **jamais** par le pipeline de routing normal. Elle lit le fichier, et si ça rate, elle génère le HTML built-in. Point.

Ta page built-in :
```cpp
std::string defaultErrorBody(int code) {
    std::ostringstream o;
    o << "<!DOCTYPE html><html><head><title>" << code << " " << reason(code)
      << "</title></head><body><center><h1>" << code << " " << reason(code)
      << "</h1></center><hr><center>webserv/1.0</center></body></html>";
    return o.str();
}
```

## 11. Les valeurs par défaut

Une config minimale doit démarrer :
```nginx
server { listen 8080; location / { root ./www; } }
```

| Directive | Défaut |
|---|---|
| `host` | `0.0.0.0` |
| `client_max_body_size` | 1 Mo |
| `index` | `index.html` |
| `autoindex` | `off` |
| `allow_methods` | `GET` (le plus restrictif) |
| `error_page` | pages built-in |
| `upload_store` | vide = upload interdit |

L'héritage server → location : la location écrase, sinon elle hérite. Le plus simple est de **résoudre l'héritage à la fin du parsing** et de remplir chaque `LocationConfig` complètement. Le reste du code lit les valeurs sans jamais se demander d'où elles viennent.

## 12. Ce que ton validateur doit refuser

Écris ces confs dans `conf/bad/`, une par fichier, et fais-en un test :

```nginx
listen 99999;                    # port > 65535
listen 0;                        # port 0
listen abc;                      # pas un nombre
server { }                       # aucun listen
location / { root /nope; }       # root inexistant
allow_methods GETT;              # méthode inconnue
return 999 /x;                   # code hors 3xx
client_max_body_size 10MB;       # suffixe invalide
autoindex maybe;                 # pas on/off
error_page abc /x.html;          # code non numérique
location /a { } location /a { }  # location dupliquée
server { listen 8080; } server { listen 8080; }   # même port, ok si server_name différent
# accolade non fermée
# ';' manquant
```

Chacune doit produire un message clair (fichier, ligne, ce qui cloche) et un `exit(1)`. Aucune ne doit être acceptée en silence.

Deux `listen 8080` dans deux blocs `server` : c'est **légal** (virtual hosts), tu ne dois créer qu'**une** socket et router sur le `Host`. Sans virtual hosts, tu prends le premier bloc. Ce qui est illégal, c'est deux `bind` sur le même port.

## 13. À retenir

- Tokenizer, parser, validateur : trois étapes séparées.
- `{`, `}`, `;` sont des tokens indépendants.
- Longest prefix match **avec frontière de segment**.
- Le sujet décrit un comportement `alias`, pas `root`. Suis le sujet et documente-le.
- Ordre du routing : slash → index → autoindex → 403.
- La page d'erreur ne passe jamais par le routing normal (récursion = crash = 0).
- Résous l'héritage à la fin du parsing, une fois pour toutes.
- Dédoublonne les paires interface:port avant de créer les sockets.
- Échappe le HTML et URL-encode les liens dans l'autoindex.

## 14. Exercice

Prends ta config cible. Écris à la main la liste des `(URI, location attendue, chemin disque attendu)` pour 15 URIs, dont `/uploadsecret`, `/kapouet/pouic/toto/pouet`, `/`, `/cgi-bin/x.py?a=1`. C'est ta suite de tests de `resolve()` et `buildPath()`, et tu l'écris **avant** le code.
