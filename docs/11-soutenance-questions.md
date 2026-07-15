# 11 — Soutenance : 60 questions et leurs réponses

Le sujet est clair : « you must demonstrate real understanding ». Et vous êtes trois : le correcteur peut poser une question sur le module CGI à celui qui a fait la config. **Chacun doit pouvoir répondre à tout.** C'est la règle qui fait échouer les groupes qui se sont mal parlés.

Format : la question, puis ce qu'il faut avoir dans la tête. Interroge-toi mutuellement une semaine avant.

## Réseau

**1. C'est quoi une socket ?**
Un point de terminaison de communication. Concrètement, un fd qui référence une structure noyau contenant les buffers d'émission/réception et l'état TCP. Un socket TCP connecté est identifié par le quadruplet (IP src, port src, IP dst, port dst) — c'est ce qui permet à 500 clients d'avoir le même port de destination.

**2. Différence entre bind, listen et accept ?**
`bind` attache une adresse à la socket. `listen` la passe en mode passif et crée la file d'attente de connexions. `accept` retire une connexion de cette file et rend une **nouvelle** socket, déjà connectée. La socket d'écoute continue son travail.

**3. Le backlog de listen, c'est quoi ?**
La taille de la file des connexions dont le handshake TCP est fini mais que tu n'as pas encore acceptées. Trop petit, tu perds des connexions sous charge. On met `SOMAXCONN` ou 128.

**4. Pourquoi SO_REUSEADDR ?**
Une socket fermée reste en `TIME_WAIT` ~60 s (2×MSL), pour que d'éventuels paquets retardataires ne polluent pas une nouvelle connexion sur le même quadruplet. Sans `SO_REUSEADDR`, un redémarrage immédiat donne `EADDRINUSE`.

**5. Pourquoi htons ?**
Le réseau est big-endian, x86 est little-endian. Sans conversion, le port 8080 devient 36895.

**6. INADDR_ANY vs 127.0.0.1 ?**
`0.0.0.0` écoute sur toutes les interfaces. `127.0.0.1` uniquement sur la loopback : inaccessible depuis l'extérieur.

**7. TCP vs UDP, pourquoi TCP ici ?**
TCP est fiable, ordonné, orienté connexion, avec contrôle de flux et de congestion. HTTP en a besoin : une requête tronquée ou désordonnée n'a aucun sens. UDP est sans connexion, non fiable — bon pour le DNS ou la vidéo, pas pour ça. (HTTP/3 est sur UDP, mais QUIC réimplémente la fiabilité par-dessus.)

**8. `recv` rend 0, ça veut dire quoi ?**
Le pair a fermé proprement : un FIN a été reçu, il n'y aura plus rien. Ce n'est **pas** « rien à lire pour l'instant ».

**9. `send` rend moins que ce que tu demandes, pourquoi ?**
Le buffer d'émission du noyau est plein. Le reste doit rester dans ton buffer de sortie jusqu'au prochain POLLOUT. C'est pour ça qu'on ne peut pas supposer que `send` écrit tout.

**10. C'est quoi SIGPIPE et pourquoi ça vous concerne ?**
Écrire sur une socket ou un pipe dont le pair a fermé déclenche SIGPIPE, qui tue le process par défaut. Un `curl` interrompu tuerait notre serveur. On fait `signal(SIGPIPE, SIG_IGN)` au démarrage ; `send` rend alors -1 et on ferme la connexion.

## Multiplexing

**11. Pourquoi poll et pas un thread par client ?**
Le sujet l'exige, mais surtout : 10 000 threads = 10 Go de stack + un coût de context switch énorme. L'event loop sert 10 000 connexions avec un thread et quelques Mo. C'est le problème C10k, et c'est le modèle de nginx, Node et Redis.

**12. select, poll, epoll, kqueue : différences ?**
`select` est limité à 1024 fds et réécrit les fd_set à chaque appel. `poll` n'a pas de limite mais reste en O(n) : le tableau est recopié userland→noyau à chaque tour. `epoll` (Linux) et `kqueue` (BSD) gardent la liste dans le noyau et ne rendent que les fds prêts : coût proportionnel à l'activité, pas au nombre de connexions.

**13. Pourquoi vous avez choisi poll ?**
Portable Linux et macOS, pas de limite à 1024, API simple, et à notre échelle la différence de perf avec epoll est invisible. epoll aurait imposé une abstraction pour compiler sur les deux OS sans gain mesurable ici.

**14. Level-triggered vs edge-triggered ?**
LT (poll, epoll par défaut) : tant qu'il reste des données, le fd est signalé à chaque tour. ET (`EPOLLET`) : signalé une seule fois au changement d'état — ce qui oblige à lire jusqu'à `EAGAIN`, donc à consulter errno, ce que le sujet interdit. Une raison de plus de rester sur poll.

**15. Non-bloquant, ça change quoi ?**
Un appel qui devrait attendre retourne immédiatement avec -1 au lieu d'endormir le thread. C'est la garantie qu'aucun client ne peut nous figer.

**16. Si poll dit que c'est prêt, pourquoi encore O_NONBLOCK ?**
Ceinture et bretelles. Il existe des races : un paquet signalé prêt peut être rejeté au dernier moment (checksum invalide), et le fd n'est plus prêt à l'appel suivant. En bloquant, ce cas rare figerait tout le serveur. Et le sujet l'exige.

**17. Pourquoi un seul recv et pas une boucle jusqu'à EAGAIN ?**
Deux raisons. La boucle oblige à lire errno pour savoir quand sortir — interdit. Et un client qui envoie en continu nous garde dans la boucle, affamant les autres. Un événement poll = une syscall ; s'il reste des données, poll nous le redira au tour suivant.

**18. Comment vous savez qu'une requête est complète ?**
Le parser est une machine à états. Il consomme les octets, avance, et rend `INCOMPLETE`, `COMPLETE` ou `ERROR`. La complétude vient du framing HTTP : `\r\n\r\n` pour les headers, puis `Content-Length` ou le chunk de taille 0 pour le body.

**19. Pourquoi POLLOUT n'est pas toujours armé ?**
Une socket est presque toujours prête en écriture. Armé en permanence, `poll()` retournerait instantanément à chaque tour et le process bouffe 100% CPU en idle. On l'arme uniquement si le buffer de sortie est non vide.

**20. Que se passe-t-il si un client se connecte et n'envoie rien ?**
Notre timeout de headers (15 s) se déclenche : 408 puis close. Sans ça, on est vulnérable au slowloris.

**21. Comment vous gérez les timeouts avec poll ?**
Chaque connexion garde un `time_t` de dernière activité. Le timeout de `poll()` est calculé sur le prochain deadline (ou fixé à 1000 ms), et un `checkTimeouts()` à chaque tour ferme les expirées.

**22. Pourquoi ne pas mettre timeout = -1 dans poll ?**
On ne serait jamais réveillés en l'absence de trafic, et les connexions expirées ne seraient jamais fermées.

## HTTP

**23. Que se passe-t-il entre la barre d'URL et l'affichage ?**
DNS, handshake TCP (SYN/SYN-ACK/ACK), envoi de la requête HTTP, notre `accept` sur POLLIN, `recv`, parsing, routing, lecture du fichier, réponse, `send` sur POLLOUT, rendu. Puis le navigateur redemande les ressources liées sur les mêmes connexions keep-alive.

**24. Différences HTTP/1.0 et 1.1 ?**
Keep-alive par défaut, `Host` obligatoire (ce qui rend les virtual hosts possibles), et `Transfer-Encoding: chunked`. On répond en 1.1 parce que c'est ce que parlent les navigateurs.

**25. Pourquoi Host est obligatoire en 1.1 ?**
En 1.0 la requête ne contient qu'un chemin : le serveur ignore quel nom de domaine était visé, donc impossible d'héberger plusieurs sites sur une IP. `Host` résout ça. S'il manque en 1.1 → 400.

**26. Content-Length ou chunked, à quoi ça sert ?**
À délimiter le body. TCP est un flux sans frontière de message ; il faut que le protocole applicatif dise où le body s'arrête. `Content-Length` quand on connaît la taille, chunked quand le contenu est généré à la volée.

**27. Comment marche le chunked ?**
Chaque chunk : taille en **hexadécimal**, CRLF, les données, CRLF. Un chunk de taille 0 termine, suivi de trailers optionnels et d'un CRLF vide. Le CRLF après les données n'est pas compté dans la taille.

**28. Que faites-vous si CL et TE sont présents ?**
400. C'est le vecteur du request smuggling : un proxy croit l'un, le serveur croit l'autre, et on injecte une requête fantôme dans la connexion de quelqu'un d'autre. La RFC 9112 §6.1 l'interdit.

**29. Différence entre 401, 403 et 404 ?**
401 : non authentifié, il faut se connecter. 403 : authentifié mais pas autorisé — ou droits UNIX insuffisants. 404 : la ressource n'existe pas. On ne gère pas l'auth, donc pas de 401.

**30. Différence entre 405 et 501 ?**
405 : méthode connue mais non autorisée sur cette route (`allow_methods`). Le header `Allow` est obligatoire. 501 : méthode qu'on n'implémente pas du tout, comme PUT ou TRACE.

**31. Pourquoi 404 et pas 500 sur un fichier absent ?**
4xx = la faute vient de la requête, 5xx = la faute vient du serveur. Une ressource inexistante, c'est la demande du client qui est fautive.

**32. Différence entre 301 et 302 ?**
301 permanent : le navigateur met en cache et ira directement à la nouvelle URL les fois suivantes. 302 temporaire : il redemande à chaque fois.

**33. GET est idempotente, ça veut dire quoi ?**
La refaire n'a pas d'effet supplémentaire. GET est aussi **sûre** : elle ne modifie rien côté serveur. DELETE est idempotente mais pas sûre. POST n'est ni l'un ni l'autre.

**34. Pourquoi un Content-Length faux est grave ?**
Trop grand : le navigateur attend des octets qui n'arrivent jamais, la page pend. Trop petit : le body est tronqué, et en keep-alive le surplus est interprété comme le début de la requête suivante — le flux est désynchronisé.

**35. Comment vous gérez le keep-alive ?**
La connexion reste ouverte, le parser est réinitialisé — **sans vider le buffer d'entrée**, parce que le reliquat peut déjà contenir la requête suivante (pipelining). Un timeout idle ferme les connexions inactives.

**36. Le pipelining ?**
Le client envoie plusieurs requêtes sans attendre les réponses. On doit répondre dans l'ordre. Ça marche naturellement si le parser traite le reliquat et si le buffer de sortie est une concaténation.

**37. Comment vous choisissez le Content-Type ?**
Une map extension → type MIME, remplie à l'init. Défaut `application/octet-stream`, qui fait télécharger plutôt qu'afficher — le comportement sûr.

## Config et routing

**38. Comment vous choisissez la location ?**
Le préfixe le plus long qui matche, **avec vérification de frontière de segment** : `/upload` ne doit pas matcher `/uploadsecret`, sinon on sert depuis la mauvaise racine — c'est une faille, pas juste un bug.

**39. Deux blocs server sur le même port, vous faites quoi ?**
Une seule socket. On route sur le header `Host` contre les `server_name`. Sans match, le premier bloc du port est le serveur par défaut — comme nginx.

**40. Comment `/kapouet/pouic/toto/pouet` devient un chemin ?**
La location `/kapouet` a `root /tmp/www`. On retire le préfixe de la location et on concatène : `/tmp/www/pouic/toto/pouet`. C'est l'exemple exact du sujet.

**41. C'est le comportement de root ou d'alias chez nginx ?**
D'`alias`. Le `root` de nginx concatènerait l'URI complète et donnerait `/tmp/www/kapouet/pouic/toto/pouet`. On a suivi le sujet, qui fait autorité, et c'est documenté dans le README.

**42. Que faites-vous sur GET /dir sans slash ?**
301 vers `/dir/`. Sans ça, les liens relatifs dans la page se résolvent au mauvais niveau et toutes les images cassent. C'est ce que fait nginx.

**43. Ordre de résolution sur un dossier ?**
Slash final d'abord (301 sinon), puis les fichiers `index`, puis l'autoindex, puis 403.

**44. Et si votre page d'erreur 404 custom n'existe pas ?**
On tombe sur la page built-in. Surtout pas d'appel récursif vers `error(404)` : ce serait une récursion infinie, un stack overflow, un crash — et un 0. La fonction qui sert une erreur ne repasse jamais par le routing normal.

**45. Comment vous validez client_max_body_size ?**
Parsing maison : que des chiffres, un suffixe optionnel k/m/g, détection d'overflow. `10MB`, `-5`, `abc` sont rejetés au démarrage avec un message clair et `exit(1)`.

## CGI

**46. C'est quoi un CGI ?**
Common Gateway Interface, RFC 3875. Le serveur lance l'interpréteur comme un process normal et lui passe la requête par les variables d'environnement et stdin ; la réponse revient sur stdout. C'est l'ancêtre de FastCGI et WSGI.

**47. Pourquoi deux pipes ?**
Un pipe est unidirectionnel. Il en faut un pour le body vers stdin du CGI, un pour la sortie du CGI vers nous.

**48. Que se passe-t-il si vous ne fermez pas out[1] dans le parent ?**
Le pipe a toujours un écrivain — nous. Quand le CGI ferme son stdout, le refcount passe de 2 à 1, pas à 0. `read` ne rend jamais 0, on attend un EOF qui n'arrivera jamais. C'est le bug CGI classique.

**49. Pourquoi exit après execve ?**
`execve` ne revient que s'il échoue. Sans `exit`, l'enfant continue dans le code du serveur : deux boucles poll sur les mêmes fds. Chaos.

**50. Pourquoi WNOHANG ?**
Sans, `waitpid` bloque jusqu'à la mort du CGI. Un script en boucle infinie figerait le serveur entier. Avec `WNOHANG`, il rend 0 si l'enfant tourne encore.

**51. Et le CGI qui ne finit jamais ?**
Timeout (10 s), `kill(SIGKILL)`, `waitpid` pour reaper, 504 Gateway Timeout. SIGKILL et pas SIGTERM, parce que SIGTERM est interceptable.

**52. C'est quoi un zombie ?**
Un process terminé dont le parent n'a pas lu le statut. Il n'occupe pas de RAM mais une entrée dans la table des processus. À quelques milliers, `fork` échoue. On reape tous nos enfants, y compris ceux qu'on tue.

**53. Pourquoi chdir dans l'enfant ?**
Pour que les chemins relatifs du script (`open("data.txt")`) fonctionnent. Le sujet l'exige. Dans l'enfant seulement — un chdir dans le parent casserait tous nos chemins de config.

**54. Comment le CGI sait où finit le body ?**
On ferme son stdin après avoir tout écrit. Le `read()` du script rend 0. C'est ce que dit le sujet : « the CGI will expect EOF as the end of the body ». Si on ne fermait pas, un script qui lit stdin attendrait pour toujours.

**55. Et si le CGI ne renvoie pas de Content-Length ?**
On lit son stdout jusqu'à l'EOF du pipe, on connaît donc la taille exacte, et on la met dans notre réponse HTTP. Le client, lui, ne peut pas deviner.

**56. CONTENT_LENGTH, c'est le HTTP_CONTENT_LENGTH ?**
Non. `Content-Length` et `Content-Type` sont les deux exceptions à la règle `HTTP_*` : elles n'ont pas le préfixe.

**57. Pourquoi pas execve("/bin/sh", "-c", ...) ?**
Injection de commande gratuite. On appelle l'interpréteur directement avec un argv construit : jamais de shell entre nous et le script.

## Robustesse et sécurité

**58. Comment vous êtes sûrs de ne jamais crasher ?**
Un `try/catch(...)` autour du corps de la boucle poll : une exception ferme une connexion au lieu de tuer le process, `bad_alloc` compris — le sujet mentionne explicitement l'épuisement mémoire. Tous les retours de syscall sont testés. Pas de `substr` sans vérifier les bornes. Et 50 000 requêtes fuzz sous valgrind.

**59. Comment vous empêchez `/../../etc/passwd` ?**
On décode l'URI **une seule fois**, on rejette les octets nuls, on canonicalise le chemin résolu, et on vérifie qu'il reste sous le root **avec vérification de frontière** (`/var/www` est un préfixe de `/var/wwwEVIL`). Filtrer la chaîne `..` ne suffit pas : `%2e%2e%2f` la contourne, d'où l'ordre décoder-puis-vérifier.

**60. Le slowloris, vous connaissez ?**
500 connexions qui envoient un header toutes les 15 secondes. Chaque requête est légale, elle n'est juste jamais finie. On sature les fds et le serveur meurt sans un seul paquet malveillant. Notre défense : timeout de headers à 15 s, plafond de 8 Ko sur les headers, et une limite de connexions.

## Les questions vicieuses

**Vous êtes trois : qui a fait quoi ?**
Réponds au niveau des modules, mais **enchaîne** en montrant que tu peux parler des autres. « J'ai fait la config, mais j'ai relu tout le CGI — d'ailleurs le piège du refcount des pipes, on l'a eu. » C'est ce qu'ils veulent entendre.

**Montrez-moi un endroit où vous ne respectez pas la RFC.**
Aie une réponse. Chaque serveur diverge quelque part. « Les virtual hosts, hors scope selon le sujet, mais on a gardé `server_name` dans la config » ou « on ne gère pas les headers multiligne, dépréciés par la RFC 9112 ». Ne réponds jamais « nulle part » : c'est faux, et ça montre que tu n'as pas cherché.

**Ce code, expliquez-le-moi ligne par ligne.** *(il pointe une fonction au hasard, potentiellement d'un autre module)*
La vraie raison de la relecture croisée. Si trois personnes n'ont jamais lu le code des deux autres, le groupe se plante ici.

**Modification demandée** *(le sujet la prévoit)*
Typiquement : ajouter un header à toutes les réponses, changer un code de statut, ajouter une directive à la config, ajouter une méthode. Une archi propre rend ça faisable en 5 minutes. Si tu dois toucher à 6 fichiers pour ajouter un header, ton découpage a un problème — et l'exercice le révèle.

## Une semaine avant

- [ ] Chacun a relu les deux autres modules **en entier**
- [ ] Chacun peut répondre aux 60 questions ci-dessus
- [ ] Vous vous êtes interrogés mutuellement, à voix haute, sans notes
- [ ] Le README est en anglais, première ligne en italique avec les trois logins
- [ ] La section « how AI was used » est écrite et honnête
- [ ] La démo est scriptée : une commande par feature du sujet
- [ ] `/kapouet/pouic/toto/pouet` est dans la config de démo et testé
- [ ] `make re` compile sans un warning avec `-std=c++98`
- [ ] Le siege tourne à 100,00 %, trois fois de suite
- [ ] `top` montre 0,0 % au repos
- [ ] `ps aux | grep defunct` est vide après 1000 requêtes CGI
- [ ] Le script CGI en boucle infinie donne 504 et le serveur survit
- [ ] Vous avez testé sur Chrome **et** Firefox
- [ ] Quelqu'un a essayé de casser le serveur pendant 20 minutes sans y arriver
