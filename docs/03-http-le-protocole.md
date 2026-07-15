# 03 — HTTP, le protocole

## 1. Une requête, une réponse, en texte

HTTP est du texte ASCII avec une structure rigide. Tu peux le taper à la main :

```
GET /index.html HTTP/1.1\r\n
Host: localhost:8080\r\n
User-Agent: curl/8.4.0\r\n
Accept: */*\r\n
\r\n
```

Structure :
```
<méthode> SP <request-target> SP <version> CRLF
<Nom-Header>: <valeur> CRLF
<Nom-Header>: <valeur> CRLF
CRLF                       <- la ligne vide sépare headers et body
[body]
```

La réponse est symétrique, la première ligne près :
```
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 1234\r\n
\r\n
<!DOCTYPE html>...
```

**`\r\n` et pas `\n`.** CRLF, deux octets, 0x0D 0x0A. Un `\n` seul est toléré par beaucoup de serveurs par indulgence, mais tes réponses doivent être en CRLF strict, sinon les navigateurs font n'importe quoi. Et c'est le genre de détail que le correcteur vérifie avec `curl -v` ou un hexdump.

## 2. Les versions

| Version | Année | Ce qui change | Pour toi |
|---|---|---|---|
| 0.9 | 1991 | `GET /path` et c'est tout. Pas de headers, pas de codes. | Anecdote |
| **1.0** | 1996 | Headers, codes, POST. Une connexion = une requête. | **Ta référence** (RFC 1945, le sujet le dit) |
| **1.1** | 1997 | Keep-alive par défaut, `Host` obligatoire, chunked, pipelining | **Ce que parle ton navigateur** |
| 2 | 2015 | Binaire, multiplexé, HPACK | Hors scope |
| 3 | 2022 | Sur QUIC/UDP | Hors scope |

**Le point qui compte.** Le sujet suggère HTTP/1.0 comme référence, mais **ton navigateur envoie du 1.1**. Tu dois donc au minimum :
- accepter `HTTP/1.1` dans la request line ;
- gérer `Connection: keep-alive` (défaut en 1.1) et `Connection: close` ;
- gérer `Transfer-Encoding: chunked` en entrée (le sujet l'exige explicitement pour le CGI) ;
- exiger le header `Host` en 1.1 → sinon **400**.

Réponds en `HTTP/1.1`. C'est cohérent avec ce que tu supportes.

### Les trois différences 1.0/1.1 à savoir en soutenance

**① Keep-alive.** En 1.0, la connexion se ferme après chaque réponse. Une page avec 30 images = 30 handshakes TCP. En 1.1, la connexion reste ouverte par défaut — d'où ton timeout idle et le reset de ton parser.

**② `Host` obligatoire.** En 1.0 l'URL demandée est juste un chemin, le serveur ne sait pas quel nom de domaine tu visais. Impossible d'héberger deux sites sur une IP. En 1.1, `Host: example.com` est obligatoire → c'est ce qui rend les virtual hosts possibles. (Hors scope selon le sujet, mais 30 lignes si ta config a déjà `server_name`, et c'est un bon point.)

**③ Chunked.** En 1.0, il faut connaître `Content-Length` à l'avance. Impossible de streamer un contenu généré à la volée. Le chunked résout ça.

## 3. Les méthodes

Le sujet en exige trois. Les autres → **501 Not Implemented** (pas 405 : 405 c'est « connue mais interdite sur cette route »).

| Méthode | Sûre ? | Idempotente ? | Body ? | Ce que tu fais |
|---|---|---|---|---|
| **GET** | oui | oui | non | Sert le fichier / autoindex / CGI |
| **POST** | non | non | oui | Upload, CGI |
| **DELETE** | non | oui | non | Supprime le fichier |
| HEAD | oui | oui | non | GET sans body — 5 lignes, fais-le |
| PUT, PATCH, OPTIONS, TRACE, CONNECT | — | — | — | 501 |

**Sûre** = ne modifie rien côté serveur. **Idempotente** = la refaire 10 fois donne le même état final. Deux notions qui tombent à la soutenance.

> `HEAD` mérite un mot : c'est un GET dont on jette le body, mais **les headers doivent être identiques**, `Content-Length` compris. Donc tu calcules tout et tu n'envoies pas le body. `curl -I` l'utilise, et un correcteur qui tape `curl -I` sur ton serveur, ça arrive.

### Codes de statut : les 15 dont tu as besoin

| Code | Nom | Quand |
|---|---|---|
| 200 | OK | GET/DELETE réussi |
| 201 | Created | Upload réussi (+ header `Location`) |
| 204 | No Content | DELETE réussi sans body. **Interdit d'avoir un body.** |
| 301 | Moved Permanently | Redirection config, `/dir` → `/dir/` |
| 302 | Found | Redirection temporaire |
| 400 | Bad Request | Parsing raté, `Host` absent en 1.1, CL+TE ensemble |
| 403 | Forbidden | Droits UNIX, ou dossier sans index avec autoindex off |
| 404 | Not Found | Le chemin n'existe pas |
| 405 | Method Not Allowed | Méthode connue mais pas dans `allow_methods`. **Header `Allow` obligatoire.** |
| 408 | Request Timeout | Le client traîne |
| 413 | Content Too Large | Body > `client_max_body_size` |
| 414 | URI Too Long | URI > ta limite (8 Ko typique) |
| 500 | Internal Server Error | Ton bug, ou le CGI a planté |
| 501 | Not Implemented | Méthode inconnue |
| 504 | Gateway Timeout | Le CGI a dépassé le délai |
| 505 | HTTP Version Not Supported | `HTTP/2.0` dans la request line |

Les classes : 1xx info, 2xx succès, 3xx redirection, 4xx erreur client, 5xx erreur serveur. **La question qui tombe** : « pourquoi 404 et pas 500 quand le fichier n'existe pas ? » → parce que le client a demandé une ressource inexistante, c'est sa demande qui est fautive, pas ton serveur. 4xx = « c'est toi », 5xx = « c'est moi ».

## 4. Les headers

Insensibles à la casse pour le nom (`content-length` == `Content-Length`) — donc stocke-les normalisés en minuscules dans ta `map`. Sensibles à la casse pour la valeur.

### Ceux que tu dois lire

| Header | Pourquoi |
|---|---|
| `Host` | Obligatoire en 1.1. Absent → 400. Sert aux virtual hosts. |
| `Content-Length` | Taille du body en octets |
| `Transfer-Encoding: chunked` | Body en chunks, taille inconnue à l'avance |
| `Content-Type` | `multipart/form-data; boundary=...` pour les uploads |
| `Connection` | `keep-alive` / `close` |
| `Cookie` | Bonus sessions |

### Ceux que tu dois écrire

| Header | Toujours ? | Note |
|---|---|---|
| `Content-Length` | oui (sauf 204/304) | **Le plus important.** Faux = navigateur qui pend ou body tronqué. |
| `Content-Type` | dès qu'il y a un body | Sinon le navigateur devine, mal |
| `Date` | recommandé | Format RFC 7231, **en GMT**, `%a, %d %b %Y %H:%M:%S GMT` |
| `Server` | optionnel | `Server: webserv/1.0` |
| `Connection` | oui | `keep-alive` ou `close` |
| `Location` | 3xx et 201 | La cible |
| `Allow` | 405 | **Obligatoire.** `Allow: GET, POST` |

`Date` en GMT : utilise `gmtime()` + `strftime()`, jamais `localtime()`. Un correcteur en Suisse verrait +2h et poserait la question.

## 5. Le body : trois façons de savoir où il s'arrête

**① `Content-Length: 1234`** → tu lis exactement 1234 octets. Simple.

**② `Transfer-Encoding: chunked`** :
```
Transfer-Encoding: chunked\r\n
\r\n
7\r\n
Mozilla\r\n
9\r\n
Developer\r\n
0\r\n
\r\n
```
Chaque chunk : taille **en hexadécimal**, CRLF, les données, CRLF. Un chunk de taille 0 termine, suivi de trailers optionnels puis d'un CRLF.

Pièges :
- **Hexa**, pas décimal. `a\r\n` = 10 octets, pas 10 en décimal par hasard identique... si, mais `10\r\n` = **16** octets. Le bug qui rend fou.
- La taille peut avoir des extensions après un `;` : `1a;foo=bar\r\n`. Coupe au `;`.
- Le CRLF après les données n'est **pas** dans la taille annoncée.
- **Le sujet t'oblige à dé-chunker** avant de passer au CGI, qui attend un body brut terminé par EOF.
- Un `Content-Length` **et** un `Transfer-Encoding` ensemble = **400**, sans discuter. C'est un vecteur classique de request smuggling : un proxy croit l'un, ton serveur croit l'autre, et on injecte une requête fantôme. Voir *10-securite.md*.

**③ Rien du tout** → pas de body. GET, DELETE, HEAD.

## 6. Content-Length, le tueur silencieux

Trois façons de te planter :

**Trop grand** → le navigateur attend des octets qui n'arrivent jamais, la page tourne, timeout au bout de 30 s. Symptôme classique : « ça marche en curl mais Chrome pend ».

**Trop petit** → le body est tronqué. Ton HTML s'arrête au milieu, ton image est cassée. Et pire, en keep-alive : les octets en trop sont interprétés comme le début de la requête suivante → désynchronisation complète du flux.

**Absent avec un body** → en 1.1 le navigateur ne sait pas quand s'arrêter, il attend la fermeture de connexion.

Règle : `Content-Length` = **exactement** `body.size()`. Calcule-le à partir du body réel, jamais d'une variable qui traîne. Et sur un `204`, il ne doit **pas** y en avoir.

## 7. Keep-alive et pipelining

En 1.1, la connexion reste ouverte par défaut. Conséquences pour toi :

**① Réinitialiser le parser** après chaque réponse, sans jeter le reliquat du buffer d'entrée. Si le client a déjà envoyé la requête suivante, elle est **déjà dans ton `_inBuf`**. Un `_inBuf.clear()` à la fin d'une réponse et tu perds des requêtes — bug fantôme sous charge, impossible à reproduire à la main.

**② Un timeout idle**, sinon les connexions s'accumulent jusqu'à épuisement des fds.

**③ Fermer si :** le client envoie `Connection: close`, ou tu réponds une erreur 4xx/5xx (le flux peut être désynchronisé), ou ton timeout expire.

**Le pipelining** — le client envoie 3 requêtes sans attendre les réponses — est autorisé en 1.1. Tu dois répondre **dans l'ordre**. Si ton parser traite bien le reliquat et que ton `_outBuf` est une simple concaténation, ça marche gratuitement. Sinon ça casse. C'est un bon test :

```bash
printf 'GET / HTTP/1.1\r\nHost: x\r\n\r\nGET /a.html HTTP/1.1\r\nHost: x\r\n\r\n' | nc localhost 8080
```
Deux réponses doivent sortir, dans l'ordre.

## 8. MIME types

`Content-Type` dit au navigateur quoi faire du body. Sans lui, il devine (« MIME sniffing ») — et un `.html` servi en `text/plain` s'affiche comme du code source.

Le minimum vital :
```
.html .htm  text/html
.css        text/css
.js         application/javascript
.json       application/json
.png        image/png
.jpg .jpeg  image/jpeg
.gif        image/gif
.svg        image/svg+xml
.ico        image/x-icon
.txt        text/plain
.pdf        application/pdf
(défaut)    application/octet-stream
```

Une `std::map<std::string, std::string>` remplie à l'init, matching sur l'extension. `application/octet-stream` en défaut → le navigateur télécharge au lieu d'afficher, ce qui est le comportement sûr.

Pour le texte, ajoute le charset : `text/html; charset=utf-8`. Sinon tes accents partent en carrés.

## 9. URL encoding

`GET /mon%20fichier.html?nom=jean%2Bpaul&age=25`

- `%XX` = un octet en hexa. `%20` = espace, `%2F` = `/`, `%2E` = `.`
- Dans une query string, `+` = espace (héritage des formulaires HTML)
- La query string commence au `?` et **ne fait pas partie du chemin** : `/cgi/x.py?a=1` → fichier `x.py`, `QUERY_STRING=a=1`
- Le fragment `#ancre` n'est **jamais** envoyé au serveur, le navigateur le garde

**Il faut décoder le chemin avant de toucher au disque.** Et il faut décoder **avant** de vérifier le path traversal, jamais après — sinon `%2e%2e%2f` passe ta vérification de `..` et devient `../` une fois décodé. C'est *le* bug de sécu classique, détaillé dans *10-securite.md*.

## 10. Ce que fait vraiment ton navigateur

Lance ton serveur, ouvre Chrome, regarde les logs. Tu verras des choses que curl ne fait pas :

- Une requête `GET /favicon.ico` **systématique**, même si tu ne l'as pas demandée. Réponds 404 proprement, ne crashe pas.
- Des connexions ouvertes **en avance**, sans requête, gardées ouvertes (préconnexion). Ton timeout idle doit les fermer sans paniquer.
- 6 connexions parallèles par domaine.
- Des headers que tu n'attends pas : `Sec-Fetch-Mode`, `Upgrade-Insecure-Requests`, `Accept-Encoding: gzip, deflate, br`.

Sur `Accept-Encoding` : tu **n'es pas obligé** de compresser. N'envoie juste pas de `Content-Encoding` et sers du brut. Le navigateur s'en accommode. Ce qui casse, c'est d'annoncer `Content-Encoding: gzip` sur du contenu non compressé.

## 11. À retenir

- CRLF, toujours, partout.
- `Host` absent en 1.1 → 400.
- `Content-Length` exact ou rien ne marche.
- Chunked : taille en **hexa**, dé-chunker avant le CGI, CL+TE ensemble → 400.
- 405 exige le header `Allow`. 204 interdit un body.
- Keep-alive → reset du parser **sans jeter le reliquat**.
- MIME type sur chaque body.
- Décode l'URL **avant** de vérifier le path traversal.
- Teste avec un vrai navigateur, pas seulement curl.

## 12. Exercice

```bash
telnet localhost 80    # sur un nginx local
GET / HTTP/1.0
[entrée] [entrée]
```
Lis la réponse ligne par ligne. Puis fais pareil en `HTTP/1.1` : la connexion reste ouverte, envoie une deuxième requête sur la même. Puis omets le `Host` en 1.1 et regarde nginx répondre 400. Tu viens de voir ta spec.

Ensuite `curl -v https://httpbin.org/get` et `curl -v --http1.0 localhost:8080` sur ton serveur : compare les headers, un par un.
