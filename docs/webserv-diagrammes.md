# Webserv — diagrammes pour le README

## 1. Cycle de vie d'une requête

```mermaid
sequenceDiagram
    autonumber
    participant B as Navigateur
    participant L as ListenSocket
    participant P as poll() unique
    participant C as Connection
    participant R as RequestParser
    participant RT as Router
    participant G as CgiProcess
    participant RB as ResponseBuilder

    Note over P: fds surveillés : listen_fds + client_fds + cgi_pipes

    B->>L: SYN / connect
    P-->>L: POLLIN sur listen_fd
    L->>L: accept() + O_NONBLOCK
    L->>C: nouvelle Connection(fd)
    C->>P: enregistre fd (POLLIN)

    B->>C: "POST /cgi/upload.py HTTP/1.1" (fragment 1)
    P-->>C: POLLIN
    C->>C: recv() dans inBuf
    C->>R: feed(inBuf)
    R-->>C: INCOMPLETE

    B->>C: headers + body (fragment 2)
    P-->>C: POLLIN
    C->>R: feed(inBuf)
    R->>R: dé-chunke si Transfer-Encoding: chunked
    R-->>C: COMPLETE

    C->>RT: route(request)
    RT->>RT: resolve host:port + URI vers LocationConfig
    RT->>RT: méthode autorisée ? body sous limite ?

    alt Erreur (405 / 413 / 404)
        RT->>RB: build_error(code, location)
        RB-->>C: outBuf
    else Fichier statique
        RT->>RB: build_static(path)
        RB->>RB: open + read + MIME
        RB-->>C: outBuf
    else Extension CGI
        RT->>G: start(request, location)
        G->>G: pipe() x2 + O_NONBLOCK
        G->>G: fork()
        Note right of G: enfant : dup2, chdir(script_dir), execve(interpreter, argv, envp)
        G->>P: enregistre pipe_in (POLLOUT) + pipe_out (POLLIN)

        loop Body vers le CGI
            P-->>G: POLLOUT sur pipe_in
            G->>G: write() un morceau
        end
        G->>G: close(pipe_in) -- le CGI voit EOF

        loop Sortie du CGI
            P-->>G: POLLIN sur pipe_out
            G->>G: read() dans cgiBuf
        end
        Note right of G: read() == 0 -- EOF, fin du body CGI
        G->>G: waitpid(WNOHANG)
        G->>RB: cgiBuf (headers CGI + body)
        RB->>RB: Status: -> ligne de statut, ajoute Content-Length
        RB-->>C: outBuf
    end

    C->>P: arme POLLOUT
    loop Envoi
        P-->>C: POLLOUT
        C->>B: send() partiel depuis outBuf
    end
    Note over C: outBuf vide -- désarme POLLOUT

    alt Connection: keep-alive
        C->>C: reset parser, garde le fd
    else Connection: close
        C->>P: retire fd
        C->>C: close(fd)
    end
```

**Ce que ce diagramme prouve à l'évaluateur** (et pourquoi il faut savoir le défendre) :
- un seul `poll()` gère listen, clients **et** pipes CGI ;
- aucun `recv`/`send`/`read`/`write` n'apparaît sans un `POLLIN`/`POLLOUT` juste au-dessus ;
- POLLOUT est armé seulement quand il y a quelque chose à écrire (sinon poll tourne à vide à 100% CPU) ;
- le parsing est incrémental : deux fragments, deux `feed()`, aucune hypothèse sur les frontières TCP.

## 2. State machine du parser HTTP

```mermaid
stateDiagram-v2
    direction TB
    [*] --> START

    START --> REQUEST_LINE : premier octet reçu
    REQUEST_LINE --> REQUEST_LINE : pas encore de CRLF
    REQUEST_LINE --> HEADERS : CRLF -- méthode + URI + version OK
    REQUEST_LINE --> ERROR : ligne malformée (400)
    REQUEST_LINE --> ERROR : méthode inconnue (501)
    REQUEST_LINE --> ERROR : version non supportée (505)
    REQUEST_LINE --> ERROR : URI trop longue (414)

    HEADERS --> HEADERS : ligne "Name: value" stockée
    HEADERS --> ERROR : header malformé (400)
    HEADERS --> ERROR : taille headers dépassée (431)
    HEADERS --> ERROR : Content-Length et Transfer-Encoding ensemble (400)

    HEADERS --> DONE : CRLF vide, aucun body attendu
    HEADERS --> BODY_LENGTH : CRLF vide + Content-Length > 0
    HEADERS --> CHUNK_SIZE : CRLF vide + Transfer-Encoding: chunked
    HEADERS --> ERROR : Content-Length > client_max_body_size (413)

    BODY_LENGTH --> BODY_LENGTH : reçu < Content-Length
    BODY_LENGTH --> DONE : reçu == Content-Length

    state "Chunked" as CH {
        CHUNK_SIZE --> CHUNK_DATA : taille hexa > 0
        CHUNK_DATA --> CHUNK_CRLF : n octets lus
        CHUNK_CRLF --> CHUNK_SIZE : CRLF consommé
        CHUNK_SIZE --> TRAILER : taille == 0
        TRAILER --> TRAILER : trailer header
    }

    CHUNK_SIZE --> ERROR : taille non hexa (400)
    CHUNK_DATA --> ERROR : total > client_max_body_size (413)
    TRAILER --> DONE : CRLF vide

    DONE --> [*] : passe au Router
    ERROR --> [*] : passe au ResponseBuilder (page d'erreur)

    note right of START
        feed(data, n) est appelée
        autant de fois que nécessaire.
        L'état survit entre les appels :
        aucun octet n'est jamais redemandé.
    end note

    note right of DONE
        keep-alive : l'état repart à START
        avec le reliquat de inBuf
        (pipelining possible).
    end note
```

**Trois invariants à tenir dans le code :**
1. `feed()` ne bloque jamais et ne relit jamais — elle consomme ce qu'on lui donne et rend l'état.
2. Toute transition vers `ERROR` porte un code HTTP : le parser décide du statut, pas l'appelant.
3. `client_max_body_size` est vérifiée **pendant** l'accumulation, pas après. Sinon un `Content-Length: 999999999999` te fait manger la RAM avant de répondre 413.

## 3. Où les mettre

Dans le `README.md`, section « Technical choices » (le chapitre V autorise des sections en plus). GitHub les rend directement.

Attention au rendu GitHub :
- pas de `<` `>` bruts dans les labels — `&lt;` / `&gt;` ;
- pas de `:` dans un label de `stateDiagram` sans guillemets (c'est le séparateur de transition) — d'où les `--` à la place dans les libellés ci-dessus ;
- `direction TB` sur `stateDiagram-v2` demande mermaid ≥ 9.2, ce que GitHub a. Teste sur mermaid.live avant de commit.
