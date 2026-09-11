# T-03 — Comparaison nginx

Référence : **nginx/1.24.0** (Ubuntu), port **8088**, même arborescence `www/`.  
Webserv : `conf/default.conf`, port **8080**.  
Même requête des deux côtés. **Pas de `curl -I`** (HEAD → 405 chez nous). Headers : GET + `curl -sD -`.

Les `Date`, le HTML des pages d’erreur et le header `Server` qui diffèrent ne sont pas des bugs.

## Les 10 comportements du ticket

| # | Requête | Webserv | nginx | Écart | Justification |
|---|---|---|---|---|---|
| A | `GET /uploads` (dir sans `/`) | **301** `Location: /uploads/` body vide | **301** `Location: http://127.0.0.1:8088/uploads/` + petit HTML | cosmétique | Même règle. nginx URL absolue + body ; nous URL relative (valide RFC). |
| A′ | `GET /uploads/` (autoindex) | **200** `Index of /uploads/` `text/html; charset=utf-8` | **200** listing | cosmétique (HTML) | Aligné après le fix `serveDir`. |
| B | `POST /` avec `-d ''` | **405** + `Allow: GET` + `405.html` | **405** + `405.html`, souvent **pas** de `Allow` | mineur | RFC : un 405 devrait avoir `Allow`. On est plus strict. nginx le perd souvent avec `error_page 405`. |
| B2 | `POST /` sans `Content-Length` | **411** | **405** | assumé | POST sans taille → 411 (parseur). nginx enchaîne sur la méthode. Avec `-d ''` les deux font 405. |
| C | `GET /nope` | **404** + `errors/404.html` (111 o) | **404** + **la même page** | non | `error_page 404` identique. |
| D | `GET /empty_dir/` (pas d’index, autoindex off) | **403** | **403** | cosmétique | Pas d’`error_page 403` → bodies différents. Code OK. |
| E | POST 11 Mo `/uploads/big.bin` | **413** *Payload Too Large* | **413** *Request Entity Too Large* | cosmétique | `client_max_body_size 10M`. Le code compte, pas la phrase. |
| F | `Transfer-Encoding: gzip` (sans `Content-Length`) | **501** | **501** | non | TE ≠ `chunked` → Not Implemented. (TE **et** `Content-Length` : nous 400, nginx 501.) |
| G | `GET / HTTP/1.2` | **505** | **200** | assumé | On refuse les versions hors 1.1. nginx est permissif. |
| H | `HTTP/1.1` sans `Host` | **400** | **400** | non | Host obligatoire en HTTP/1.1. |
| I | keep-alive : 2 GET, **même** socket | 200, pas de `Connection`, **1** réponse | 200 + `Connection: keep-alive`, **2** réponses | assumé | On ferme après chaque requête. HTTP/1.1 préférerait keep-alive. Les sessions (E-02) passent par le cookie, pas par le TCP. |
| J | `GET /` headers | `Date` + `Server: webserv` + `Content-Type: text/html` | idem + `Server: nginx/1.24.0` + `Connection` | cosmétique | `Server` volontaire. L’ordre des headers n’est pas normalisé. |

## Hors liste (soutenance)

| Requête | Webserv | nginx | Note |
|---|---|---|---|
| `GET /old` | 301 `Location: /new` | 301 URL absolue | `return` OK |
| `GET /ping` | 200 `pong` `text/plain` | 200 `pong` `application/octet-stream` | MIME du `return` : nginx n’en met pas |
| `GET /deny` | 403 | 403 | OK |
| `HEAD /` (`curl -I`) | **405** | **200** | Sujet = GET / POST / DELETE. Démo en GET. |
| `GET / HTTP/1.0` sans Host | **400** | **200** | On vise HTTP/1.1. |
| `GET /secret.txt` | 200 `text/plain` | 200 `text/plain` | MIME OK |
| POST chunked `/uploads/x` | **201** + `Location` | **405** | `upload_store` n’existe pas chez nginx. Voulu. |
| CGI `/cgi-bin/*.py` | 200 / CGI | pas dans la conf nginx T-03 | Hors comparaison stock |

On s’aligne sur les codes (301 slash, 404, 403, 405, 413, 501 TE, 400 Host, autoindex 200). HEAD, HTTP/1.2, 411 sans `Content-Length`, `Location` relative, `Server`, keep-alive : assumés. CGI / upload : pas des directives nginx stock.