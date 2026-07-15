# 02 — Multiplexing et non-bloquant

C'est **le** fichier du projet. Le reste est de la plomberie ; ça, c'est la note.

## 1. Le problème

Tu as 500 clients. Tu as 1 thread. Comment tu sers tout le monde ?

**Approche naïve — bloquante, mono-client :**
```cpp
while (true) {
    int fd = accept(listen_fd, NULL, NULL);
    handle(fd);      // recv() bloque jusqu'à ce qu'il y ait des données
    close(fd);
}
```
Un client lent (ou malveillant : il se connecte et n'envoie rien) fige le serveur pour tout le monde. Un seul attaquant suffit. Inacceptable.

**Approche thread par client :** interdite ici, et de toute façon 10 000 threads = 10 Go de stack et un ordonnanceur qui pleure. C'est le problème C10k.

**Approche event loop** — la seule autorisée, et la bonne :
> Ne demande jamais « donne-moi des données ». Demande « lequel de ces 500 fds a quelque chose pour moi ? », puis sers **uniquement** ceux-là, sans jamais attendre.

C'est nginx. C'est Node. C'est Redis. C'est ce que tu vas écrire.

## 2. Bloquant vs non-bloquant

```cpp
// fd bloquant
recv(fd, buf, 1024, 0);   // le thread dort dans le noyau jusqu'à ce qu'il y ait des données

// fd non-bloquant (O_NONBLOCK)
recv(fd, buf, 1024, 0);   // retourne immédiatement : soit des données, soit -1
```

On le règle avec :
```cpp
fcntl(fd, F_SETFL, O_NONBLOCK);
```

Le sujet limite `fcntl` à `F_SETFL`, `O_NONBLOCK` et `FD_CLOEXEC`. Note bien : tu ne peux pas faire `fcntl(fd, F_GETFL)` pour préserver les flags existants. Sur une socket fraîche ça n'a aucune importance — écrase.

**Attention au contresens.** Non-bloquant ne veut **pas** dire « je peux appeler recv n'importe quand sans risque ». Le sujet interdit d'appeler recv sans que poll ait dit prêt, même sur un fd non-bloquant. Les deux règles sont cumulatives :

| Règle | Vient de | Sanction |
|---|---|---|
| Les fds sont non-bloquants | Le sujet | 0 |
| Aucun read/write sans poll prêt | Le sujet | 0 |

Le non-bloquant est ta **ceinture de sécurité** contre les cas tordus (une socket peut être signalée prête et ne plus l'être à l'appel suivant — race avec un checksum invalide, par exemple). Le poll est ton **contrat**.

## 3. `poll()` en détail

```cpp
#include <poll.h>

struct pollfd {
    int   fd;        // le fd à surveiller
    short events;    // ce qui m'intéresse : POLLIN | POLLOUT
    short revents;   // ce que le noyau a trouvé (il le remplit)
};

int n = poll(struct pollfd *fds, nfds_t nfds, int timeout_ms);
```

- `fds` : un tableau. En C++98 : `std::vector<struct pollfd>` et tu passes `&v[0]`. (Pas `v.data()`, c'est C++11.)
- `nfds` : la taille du tableau.
- `timeout_ms` : `-1` = attendre indéfiniment, `0` = ne pas attendre (busy loop, à éviter), `> 0` = attendre au plus n ms.
- Retour : nombre de fds avec `revents != 0`. `0` = timeout expiré. `-1` = erreur.

**Les flags qui te servent :**

| Flag | Dans `events` ? | Signification |
|---|---|---|
| `POLLIN` | oui | Données lisibles, **ou** FIN reçu (recv retournera 0), **ou** connexion en attente sur un listen_fd |
| `POLLOUT` | oui | On peut écrire sans bloquer |
| `POLLHUP` | non, le noyau le lève seul | Le pair a raccroché |
| `POLLERR` | non, idem | Erreur sur le fd |
| `POLLNVAL` | non, idem | fd invalide — **c'est un bug chez toi** |

`POLLHUP`, `POLLERR` et `POLLNVAL` sont toujours reportés, tu n'as pas à les demander. Mais tu dois les **traiter**, sinon tu boucles sur un fd mort.

### Le squelette

```cpp
while (running) {
    int ready = poll(&_pfds[0], _pfds.size(), computeTimeout());
    if (ready < 0) {
        if (running) break;   // interrompu par un signal ou vraie erreur
        break;
    }

    // Parcours à l'envers : on peut supprimer des éléments sans casser l'index
    for (int i = _pfds.size() - 1; i >= 0; --i) {
        short re = _pfds[i].revents;
        if (re == 0)
            continue;

        if (re & (POLLERR | POLLNVAL)) { closeConnection(i); continue; }
        if (re & POLLHUP)              { closeConnection(i); continue; }

        if (re & POLLIN) {
            if (isListenFd(_pfds[i].fd))
                acceptNewClient(_pfds[i].fd);
            else if (isCgiPipe(_pfds[i].fd))
                readFromCgi(i);
            else
                readFromClient(i);   // UN recv, pas une boucle
        }
        if (re & POLLOUT) {
            if (isCgiPipe(_pfds[i].fd))
                writeToCgi(i);
            else
                writeToClient(i);    // UN send, pas une boucle
        }
    }
    checkTimeouts();
}
```

**Le parcours à l'envers** n'est pas une coquetterie : quand tu fermes une connexion tu la retires du vecteur, et si tu itères à l'endroit tu sautes un élément ou tu lis hors des clous. À l'envers, les indices déjà visités ne bougent pas.

**Piège fatal** : `_pfds` est un `vector`. Si tu ajoutes un élément pendant l'itération, il peut **réallouer** et tous tes pointeurs/références deviennent invalides. Soit tu utilises des indices (jamais des pointeurs), soit tu mets les nouveaux fds dans une file d'attente que tu vides après la boucle.

### Armer et désarmer POLLOUT

```cpp
_pfds[i].events = POLLIN;
if (!_conns[fd].outBuf.empty())
    _pfds[i].events |= POLLOUT;
```

Une socket est presque toujours prête en écriture. POLLOUT armé en permanence = `poll()` retourne instantanément à chaque tour = 100% CPU en idle. C'est le premier truc que je regarderais à ta place en soutenance.

### Le timeout de poll

Tu ne peux pas mettre `-1` si tu veux gérer les timeouts de connexion : `poll()` ne te réveillera jamais. Calcule le prochain deadline :

```cpp
int Server::computeTimeout() {
    if (_conns.empty()) return -1;
    long soonest = LONG_MAX;
    for (it = _conns.begin(); it != _conns.end(); ++it) {
        long remaining = it->second.deadline() - now_ms();
        if (remaining < soonest) soonest = remaining;
    }
    if (soonest < 0) return 0;
    return (int)soonest;
}
```

Plus simple et acceptable : un timeout fixe de 1000 ms, et `checkTimeouts()` à chaque tour. Tu réveilles le process une fois par seconde pour rien, c'est négligeable, et c'est 10 lignes au lieu de 40.

## 4. Les quatre mécanismes

| | `select` | `poll` | `epoll` (Linux) | `kqueue` (BSD/macOS) |
|---|---|---|---|---|
| Limite de fds | **1024** (FD_SETSIZE) | aucune | aucune | aucune |
| Complexité par tour | O(n) | O(n) | **O(événements)** | O(événements) |
| L'état vit où | userland, recopié à chaque appel | userland, recopié | **dans le noyau** | dans le noyau |
| Il faut réarmer | oui, les fd_set sont écrasés | non | non | non |
| Portable | partout | partout (POSIX) | Linux seul | BSD/macOS seul |

**Ce que tu dois comprendre.** `select` et `poll` recopient tout le tableau userland→noyau à chaque appel, et le noyau parcourt les n fds. À 10 000 connexions dont 3 actives, tu scannes 10 000 entrées pour trouver 3 événements. C'est le problème C10k.

`epoll` inverse : tu déclares tes fds une fois (`epoll_ctl`), le noyau garde la liste et te rend **uniquement les fds prêts** (`epoll_wait`). Coût proportionnel à l'activité, pas au nombre de connexions. C'est ce qui a rendu nginx possible.

**Ce que tu dois choisir.** `poll()`. Franchement.
- Portable Linux **et** macOS (à 42 tu as les deux).
- Pas de limite à 1024.
- L'API tient en une struct.
- À l'échelle du projet (quelques centaines de connexions au stress test), la différence de perf avec epoll est invisible.

Prends epoll seulement si tu veux le mettre sur ton CV et que tu es prêt à écrire une abstraction pour compiler sur les deux OS. Le sujet autorise, mais ne récompense pas.

> **Level-triggered vs edge-triggered** : si tu vas sur epoll, la question tombera. LT (défaut, comme poll) : tant qu'il reste des données, le fd est signalé à chaque tour. ET (`EPOLLET`) : signalé **une seule fois** au changement d'état — donc tu **dois** lire jusqu'à EAGAIN, ce qui t'oblige à lire errno... interdit par le sujet. Conclusion : si tu prends epoll, reste en level-triggered. Autre raison de rester sur poll.

## 5. L'anatomie d'une connexion

Chaque client a un état. Sans lui, tu ne peux pas être non-bloquant.

```cpp
class Connection {
private:
    int             _fd;
    std::string     _inBuf;      // octets reçus, pas encore parsés
    std::string     _outBuf;     // octets à envoyer, pas encore partis
    RequestParser   _parser;     // sa state machine à lui
    Response        _response;
    time_t          _lastActivity;
    bool            _keepAlive;
    CgiProcess*     _cgi;        // NULL si pas de CGI en cours
    // ...
};
```

`std::map<int, Connection>` indexé par fd, et tu retrouves tout en O(log n).

**Pourquoi les buffers sont obligatoires** : tu as reçu une demi-requête. Tu ne peux pas la traiter. Tu ne peux pas attendre la suite (ce serait bloquer). Donc tu la **stockes** et tu retournes dans le poll. Idem en sortie : tu as 50 Ko à envoyer, `send` en a pris 8, les 42 restants doivent vivre quelque part jusqu'au prochain POLLOUT.

Un serveur non-bloquant, c'est une machine à états + des buffers. Il n'y a rien d'autre.

## 6. Un événement = une syscall

```cpp
// FAUX
while ((n = recv(fd, buf, sizeof(buf), 0)) > 0)
    _inBuf.append(buf, n);
// -> te force à lire errno pour sortir (interdit)
// -> un client qui envoie en continu te garde dans la boucle : famine des autres

// JUSTE
ssize_t n = recv(fd, buf, sizeof(buf), 0);
if (n > 0)       _inBuf.append(buf, n);
else if (n == 0) closeConnection(fd);   // FIN
else             closeConnection(fd);   // erreur, sans errno
// puis on retourne dans le poll
```

S'il reste des données, `poll()` te le redira au tour suivant (level-triggered). Tu ne perds rien. Tu gagnes l'équité entre clients et la conformité au sujet.

Même logique pour `send` : **un** send par POLLOUT.

## 7. Les timeouts

Le sujet : « A request to your server should never hang indefinitely. »

Trois timeouts à avoir :

| Timeout | Durée typique | Déclenche |
|---|---|---|
| Headers | 10-30 s | 408 Request Timeout puis close |
| Body | 30-60 s | 408 puis close |
| Idle keep-alive | 5-75 s | close silencieux, sans réponse |
| CGI | 5-10 s | kill(SIGKILL) + 504 Gateway Timeout |

Sans le timeout de headers, tu es vulnérable au **slowloris** : j'ouvre 500 connexions, j'envoie `GET / HTTP/1.1\r\n` et un header toutes les 20 secondes. Chaque connexion reste ouverte des heures, ton serveur sature ses fds, et il ne sert plus personne. Zéro paquet malveillant, zéro bande passante. Voir *10-securite.md*.

L'implémentation est triviale : `_lastActivity = time(NULL)` à chaque recv/send, et un `checkTimeouts()` à chaque tour de boucle qui ferme les connexions expirées.

## 8. Les pipes CGI dans le même poll

Le point que tout le monde rate.

Le sujet dit « 1 poll() for all the I/O operations » et « I/O that can wait for data (sockets, pipes/FIFOs, etc.) must be non-blocking and driven by a single poll() ». Les pipes du CGI ne sont **pas** une exception.

Ce qui est interdit :
```cpp
// NON
fork();
write(pipe_in, body.c_str(), body.size());     // pipe plein à 64 Ko -> tu bloques
close(pipe_in);
while ((n = read(pipe_out, buf, 4096)) > 0)    // bloque tant que le CGI calcule
    output.append(buf, n);
waitpid(pid, &status, 0);                       // bloque jusqu'à la fin
```
Quatre blocages. Un script qui met 3 secondes fige tes 500 clients pendant 3 secondes. Et un `while (True: pass)` fige le serveur pour toujours.

Ce qu'il faut :
```cpp
// pipes en O_NONBLOCK, ajoutés au vecteur _pfds
// pipe_in  surveillé en POLLOUT : on écrit le body morceau par morceau
// pipe_out surveillé en POLLIN  : on lit la sortie morceau par morceau
// read() == 0 sur pipe_out -> EOF -> le CGI a fini d'écrire
// waitpid(pid, &status, WNOHANG) -> jamais bloquant
```

Le CGI devient **une connexion comme une autre** dans ta boucle. C'est le bon modèle mental, et c'est ce qui rend l'architecture cohérente.

Détail : un pipe a un buffer noyau de 64 Ko. Un POST de 10 Mo vers un CGI qui lit lentement remplit le pipe et `write` retourne un envoi partiel. D'où POLLOUT et l'écriture incrémentale.

## 9. Les fichiers disque

Bonne nouvelle du sujet : « You are not required to use poll() for regular disk files; read() and write() on them do not require readiness notifications. »

Un `open()` + `read()` sur un `.html` est autorisé en direct. Techniquement un read disque peut prendre des ms (page fault, disque lent), mais `O_NONBLOCK` n'a aucun effet sur les fichiers réguliers sous Linux de toute façon — c'est pour ça que le sujet exempte.

**Le vrai piège** : distingue « fichier régulier » de « FIFO/pipe/socket ». Un `stat()` avec `S_ISREG` te le dit. Si quelqu'un met un FIFO dans ton `www/`, un `open()` bloquant dessus fige tout le serveur. Cas tordu, mais c'est exactement le genre de question que pose un correcteur curieux.

## 10. Le CPU à 100%

Trois causes, par fréquence :

1. **POLLOUT toujours armé.** La plus fréquente. Voir §3.
2. **`timeout = 0` dans poll.** Tu fais du busy-waiting explicite. Mets ≥ 1 ms.
3. **Un fd en erreur jamais fermé.** `POLLERR` levé à chaque tour, tu l'ignores, poll retourne instantanément à l'infini.

Diagnostic : `top` doit montrer **0,0%** au repos avec des connexions ouvertes. Si ce n'est pas le cas, tu as un de ces trois bugs. `strace -c ./webserv conf/default.conf` te montre le nombre d'appels à poll : s'il explose, tu as ta réponse.

## 11. À retenir

- Une event loop = machine à états + buffers. Rien d'autre.
- Un événement poll = **une** syscall. Pas de boucle.
- Ne lis jamais errno après read/write. Le retour suffit : `>0` traite, `==0` ferme, `<0` ferme.
- POLLOUT armé seulement si le buffer de sortie est non vide.
- Chaque connexion a son état, son buffer d'entrée, son buffer de sortie, son deadline.
- Les pipes CGI vont dans le même poll. Pas d'exception.
- Timeouts partout : headers, body, idle, CGI.
- 0% CPU au repos, sinon tu as un bug.

## 12. Exercice

Reprends ton echo server bloquant du fichier 01. Convertis-le en poll, 3 clients simultanés. Ouvre trois `telnet`. Tape dans le deuxième : les trois doivent rester réactifs.

Puis le test qui compte : ouvre un telnet, tape `a` sans valider, et laisse-le comme ça. Ouvre un deuxième telnet : est-ce qu'il répond ? Si oui, tu as compris. Si non, tu bloques quelque part — trouve où.
