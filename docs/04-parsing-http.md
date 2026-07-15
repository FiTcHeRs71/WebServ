# 04 — Parser du HTTP

## 1. La contrainte de base

Ton parser reçoit des morceaux arbitraires. Il doit :
- accepter n'importe quel découpage, y compris octet par octet ;
- ne jamais bloquer ni redemander des octets ;
- garder son état entre deux appels ;
- ne jamais crasher, quoi qu'on lui envoie ;
- s'arrêter au bon endroit et rendre le reliquat (pipelining).

Ça s'appelle un **parser incrémental à état**. L'interface tient en trois lignes :

```cpp
class RequestParser {
public:
    enum State { INCOMPLETE, COMPLETE, ERROR };
    State feed(const char* data, size_t len);
    const Request& request() const;
    int            errorCode() const;   // 400, 413, 501, 505...
    void           reset();             // pour le keep-alive
};
```

`feed()` consomme, avance l'état, rend le verdict. Elle est appelée autant de fois qu'il le faut. Aucun octet n'est jamais redemandé.

## 2. Les états

Voir le `stateDiagram` complet dans `webserv-diagrammes.md`. Le résumé :

```
START -> REQUEST_LINE -> HEADERS -> { DONE
                                    | BODY_LENGTH -> DONE
                                    | CHUNK_SIZE -> CHUNK_DATA -> CHUNK_CRLF -> ...
                                                 -> TRAILER -> DONE }
                                    -> ERROR (à tout moment, avec un code HTTP)
```

Deux invariants :
1. **Toute transition vers ERROR porte un code HTTP.** Le parser décide du statut, pas l'appelant. `_errorCode = 400` en même temps que `_state = ERROR`.
2. **La limite de taille est vérifiée pendant l'accumulation**, jamais après. Un `Content-Length: 999999999999` doit sortir 413 à la lecture des headers, avant le premier octet de body.

## 3. Squelette

```cpp
RequestParser::State RequestParser::feed(const char* data, size_t len)
{
    _buf.append(data, len);

    while (true) {
        switch (_state) {
            case REQUEST_LINE:
                if (!parseRequestLine()) return _state;  // INCOMPLETE ou ERROR
                break;
            case HEADERS:
                if (!parseHeaders())     return _state;
                break;
            case BODY_LENGTH:
                if (!parseBodyLength())  return _state;
                break;
            case CHUNK_SIZE:
                if (!parseChunkSize())   return _state;
                break;
            // ...
            case DONE:
            case ERROR:
                return _state;
        }
    }
}
```

Chaque sous-fonction :
- rend `false` si elle manque de données (l'état reste le même, `feed` sort en `INCOMPLETE`) ;
- consomme ce qu'elle a utilisé dans `_buf` ;
- passe à l'état suivant et rend `true` ;
- ou pose `_state = ERROR` + `_errorCode` et rend `false`.

La boucle `while(true)` permet d'enchaîner : si un seul `feed()` apporte toute la requête, on traverse tous les états sans revenir dans le poll.

## 4. La request line

```cpp
bool RequestParser::parseRequestLine()
{
    size_t pos = _buf.find("\r\n");
    if (pos == std::string::npos) {
        if (_buf.size() > MAX_REQUEST_LINE)   // 8192
            return fail(414);
        return false;   // INCOMPLETE : pas encore de CRLF
    }

    std::string line = _buf.substr(0, pos);
    _buf.erase(0, pos + 2);

    // exactement deux espaces, pas de tabulation, pas d'espaces multiples
    size_t sp1 = line.find(' ');
    if (sp1 == std::string::npos) return fail(400);
    size_t sp2 = line.find(' ', sp1 + 1);
    if (sp2 == std::string::npos) return fail(400);
    if (line.find(' ', sp2 + 1) != std::string::npos) return fail(400);

    _req.method  = line.substr(0, sp1);
    _req.target  = line.substr(sp1 + 1, sp2 - sp1 - 1);
    _req.version = line.substr(sp2 + 1);

    if (!isKnownMethod(_req.method))          return fail(501);
    if (_req.target.empty() || _req.target[0] != '/') return fail(400);
    if (_req.version != "HTTP/1.1" && _req.version != "HTTP/1.0") return fail(505);

    splitQuery(_req.target, _req.path, _req.query);   // coupe au '?'
    if (!percentDecode(_req.path, _req.decodedPath))  return fail(400);

    _state = HEADERS;
    return true;
}
```

Points :
- **La limite de taille se vérifie avant de trouver le CRLF.** Sinon je t'envoie 4 Go sans CRLF et ton `_buf` mange la RAM.
- L'ordre des vérifications compte : méthode inconnue → 501, version inconnue → 505, le reste → 400.
- La query string est séparée ici. Le CGI en aura besoin (`QUERY_STRING`).
- Le décodage `%XX` se fait ici, et la vérification du path traversal se fera **après**, sur `decodedPath`.

## 5. Les headers

```cpp
bool RequestParser::parseHeaders()
{
    while (true) {
        size_t pos = _buf.find("\r\n");
        if (pos == std::string::npos) {
            if (_headerBytes + _buf.size() > MAX_HEADERS)   // 8192
                return fail(431);
            return false;
        }

        if (pos == 0) {              // ligne vide -> fin des headers
            _buf.erase(0, 2);
            return finishHeaders();
        }

        std::string line = _buf.substr(0, pos);
        _buf.erase(0, pos + 2);
        _headerBytes += pos + 2;
        if (_headerBytes > MAX_HEADERS) return fail(431);

        size_t colon = line.find(':');
        if (colon == std::string::npos || colon == 0) return fail(400);

        std::string name = toLower(line.substr(0, colon));
        std::string val  = trim(line.substr(colon + 1));

        if (name.find_first_of(" \t") != std::string::npos) return fail(400);
        if (!isValidTokenName(name)) return fail(400);

        if (_headers.count(name))
            _headers[name] += ", " + val;   // headers répétés -> concaténés
        else
            _headers[name] = val;
    }
}
```

**`finishHeaders()`** fait les décisions :

```cpp
bool RequestParser::finishHeaders()
{
    bool hasCL = _headers.count("content-length");
    bool hasTE = _headers.count("transfer-encoding");

    if (hasCL && hasTE) return fail(400);        // request smuggling

    if (_req.version == "HTTP/1.1" && !_headers.count("host"))
        return fail(400);                        // Host obligatoire en 1.1

    _req.keepAlive = (_req.version == "HTTP/1.1");
    if (_headers.count("connection")) {
        std::string c = toLower(_headers["connection"]);
        if (c.find("close") != std::string::npos)      _req.keepAlive = false;
        else if (c.find("keep-alive") != std::string::npos) _req.keepAlive = true;
    }

    if (hasTE) {
        if (toLower(_headers["transfer-encoding"]) != "chunked")
            return fail(501);
        _state = CHUNK_SIZE;
        return true;
    }
    if (hasCL) {
        if (!parseSize(_headers["content-length"], _contentLength))
            return fail(400);                    // pas un nombre, négatif, overflow
        if (_contentLength > _maxBodySize)
            return fail(413);                    // AVANT de lire le body
        if (_contentLength == 0) { _state = DONE; return true; }
        _state = BODY_LENGTH;
        return true;
    }
    _state = DONE;   // pas de body
    return true;
}
```

Les pièges d'ici :

**`Content-Length` mal formé.** `abc`, `-5`, `99999999999999999999`, `1 2`, `+3`, ` 5 `. Un `atoi()` naïf rend 0 sur tout ça et tu traites une requête vide au lieu de répondre 400. Écris ton `parseSize` : uniquement des chiffres, non vide, détection d'overflow.

**Plusieurs `Content-Length`** avec des valeurs différentes → 400. Vecteur de smuggling.

**Espace avant le `:`** (`Content-Length : 5`) → 400. La RFC 9112 est explicite : c'est une désynchronisation potentielle.

**Header sans nom** (`: value`) → 400.

**Nom avec des caractères hors du jeu token** → 400. Les tokens autorisés : alphanum et `!#$%&'*+-.^_`|~`.

## 6. Le body à Content-Length

```cpp
bool RequestParser::parseBodyLength()
{
    size_t need = _contentLength - _req.body.size();
    size_t take = std::min(need, _buf.size());
    _req.body.append(_buf, 0, take);
    _buf.erase(0, take);

    if (_req.body.size() == _contentLength) { _state = DONE; return true; }
    return false;   // INCOMPLETE
}
```

Note le `min` : s'il y a **plus** que le body dans `_buf`, le surplus est la requête suivante (pipelining). Tu ne le touches pas, il reste dans `_buf`. Un `_buf.clear()` ici et tu perds des requêtes.

## 7. Le chunked

```cpp
bool RequestParser::parseChunkSize()
{
    size_t pos = _buf.find("\r\n");
    if (pos == std::string::npos) {
        if (_buf.size() > 32) return fail(400);   // une taille ne fait pas 32 octets
        return false;
    }

    std::string line = _buf.substr(0, pos);
    _buf.erase(0, pos + 2);

    size_t semi = line.find(';');                 // extensions de chunk
    if (semi != std::string::npos) line = line.substr(0, semi);
    line = trim(line);

    if (line.empty() || line.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
        return fail(400);

    _chunkSize = hexToSize(line);                 // AVEC détection d'overflow

    if (_chunkSize == 0) { _state = TRAILER; return true; }

    if (_req.body.size() + _chunkSize > _maxBodySize)
        return fail(413);                         // AVANT d'accumuler

    _state = CHUNK_DATA;
    return true;
}
```

Les cinq pièges du chunked :

**① Hexa, pas décimal.** `10\r\n` = **16 octets**. Un `atoi` te donne 10 et tu désynchronises tout le flux. `strtoul(s.c_str(), NULL, 16)`, ou ta propre fonction — plus sûr pour détecter l'overflow.

**② Les extensions.** `1a;ext=val\r\n` est légal. Coupe au `;`.

**③ Le CRLF après les données** n'est pas compté dans la taille. Un chunk de 5 octets occupe `5\r\nhello\r\n`, soit 12 octets dans le flux. Oublier de consommer le CRLF final décale d'un chunk.

**④ Le chunk 0 n'est pas la fin.** Il est suivi de trailers optionnels, puis d'un CRLF vide. `0\r\n\r\n` dans le cas simple, mais `0\r\nX-Checksum: abc\r\n\r\n` est légal.

**⑤ La limite se vérifie sur le cumul**, pas sur un chunk. Sinon 1000 chunks de 1 Mo passent alors que la limite est à 1 Mo.

Le test qui trouve tout :
```bash
printf 'POST /up HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n7\r\nMozilla\r\n9\r\nDeveloper\r\n0\r\n\r\n' | nc localhost 8080
```
Le body reçu doit valoir exactement `MozillaDeveloper`, 16 octets.

## 8. Le multipart/form-data

Pour les uploads depuis un `<form enctype="multipart/form-data">`. C'est un format **dans** le body, pas du HTTP.

```
Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryABC123

------WebKitFormBoundaryABC123\r\n
Content-Disposition: form-data; name="file"; filename="photo.png"\r\n
Content-Type: image/png\r\n
\r\n
<octets binaires bruts>\r\n
------WebKitFormBoundaryABC123--\r\n
```

Points :
- Le boundary vient du header `Content-Type`, après `boundary=`. Il peut être entre guillemets.
- Le délimiteur réel dans le body est `--` + boundary. Le dernier est `--` + boundary + `--`.
- **Le contenu est binaire.** Interdiction absolue de traiter ça comme une chaîne C : il y a des `\0` dedans. `std::string` est OK (il gère les `\0`), `strlen`/`strstr` non. Utilise `_buf.find(delim, pos)` de `std::string`, pas `strstr`.
- Le `\r\n` qui précède le boundary de fin appartient au boundary, **pas au fichier**. Si tu l'inclus, tes fichiers uploadés font 2 octets de trop et `diff` échoue. C'est le bug d'upload numéro un.
- `filename` doit être **sanitizé** : basename uniquement, et refuse les `..` et les `/`. Sinon je t'uploade `../../.ssh/authorized_keys`. Voir *10-securite.md*.

Test définitif :
```bash
curl -F "file=@photo.png" http://localhost:8080/upload
diff photo.png uploads/photo.png && echo OK
```
Si `diff` est vert sur un fichier binaire de plusieurs Mo, ton parser multipart est bon.

## 9. Le reset pour le keep-alive

```cpp
void RequestParser::reset()
{
    _state = REQUEST_LINE;
    _req = Request();          // vide la requête
    _headers.clear();
    _headerBytes = 0;
    _contentLength = 0;
    _chunkSize = 0;
    _errorCode = 0;
    // _buf n'est PAS vidé : le reliquat est la requête suivante
}
```

Le commentaire de la dernière ligne est la ligne la plus importante du fichier.

## 10. La checklist des saloperies à tester

Chaque ligne doit donner une réponse propre, jamais un crash, jamais un hang :

```bash
# request line
printf 'GET\r\n\r\n'                              # 400
printf 'GET /\r\n\r\n'                            # 400 (pas de version)
printf 'GET  /  HTTP/1.1\r\n\r\n'                 # 400 (double espace)
printf 'BREW / HTTP/1.1\r\nHost: x\r\n\r\n'       # 501
printf 'GET / HTTP/9.9\r\nHost: x\r\n\r\n'        # 505
printf 'GET / HTTP/1.1\r\n\r\n'                   # 400 (Host absent)
printf "GET /$(python3 -c 'print("a"*9000)') HTTP/1.1\r\nHost: x\r\n\r\n"   # 414

# headers
printf 'GET / HTTP/1.1\r\nHost: x\r\nContent-Length : 5\r\n\r\n'   # 400
printf 'GET / HTTP/1.1\r\nHost: x\r\n: v\r\n\r\n'                   # 400
printf 'POST / HTTP/1.1\r\nHost: x\r\nContent-Length: abc\r\n\r\n'  # 400
printf 'POST / HTTP/1.1\r\nHost: x\r\nContent-Length: -5\r\n\r\n'   # 400
printf 'POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 99999999999999999999\r\n\r\n'  # 400 ou 413
printf 'POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\n'  # 400
printf 'POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 5\r\nContent-Length: 6\r\n\r\n'           # 400

# chunked
printf 'POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\nZZ\r\n'   # 400

# body
# Content-Length: 1000 mais on n'envoie que 10 octets et on attend -> 408

# fragmentation
# la même requête, envoyée octet par octet avec 50ms entre chaque -> même résultat

# pipelining
printf 'GET / HTTP/1.1\r\nHost: x\r\n\r\nGET / HTTP/1.1\r\nHost: x\r\n\r\n'  # 2 réponses
```

Mets-les dans `tests/run.py`. Chaque flèche vers `ERROR` de ton state diagram = une ligne ici.

## 11. À retenir

- Parser incrémental à état. Aucune hypothèse sur le découpage.
- Chaque limite (request line, headers, body) est vérifiée **pendant** l'accumulation.
- `reset()` ne vide **pas** le buffer d'entrée.
- Chunked = hexa, extensions, CRLF hors taille, chunk 0 + trailers, limite sur le cumul.
- CL + TE ensemble = 400. Deux CL différents = 400. Espace avant `:` = 400.
- Multipart : binaire, `std::string::find`, et le `\r\n` avant le boundary de fin n'est pas dans le fichier.
- Le parser porte le code HTTP de son erreur.

## 12. Exercice

Écris `feed()` et un `main` de test qui envoie une requête complète **un octet à la fois**, en affichant l'état après chaque appel. Puis la même en un seul bloc. Puis deux requêtes collées. Les trois doivent donner exactement le même résultat.

Ce petit main est jetable, mais si tu le fais avant de brancher le réseau, tu débugues ton parser sans sockets. Ça vaut trois jours.
