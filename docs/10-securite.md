# 10 — Sécurité

Ce fichier n'est pas exigé par le sujet. Mais un serveur HTTP est *le* programme exposé par définition, et les failles ci-dessous sont des CVE réelles qui ont coûté des millions. Vu ta spécialisation cybersécurité, c'est le fichier où le projet devient intéressant — et en soutenance, savoir de quoi tu te défends, ça se voit tout de suite.

Contexte à garder en tête : tu **implémentes les défenses de ton propre serveur**. Rien ici ne sert à attaquer autre chose que ton code.

## 1. Path traversal — la faille numéro un

```
GET /../../../../etc/passwd HTTP/1.1
GET /..%2f..%2f..%2fetc%2fpasswd HTTP/1.1
GET /%2e%2e%2f%2e%2e%2fetc/passwd HTTP/1.1
GET /....//....//etc/passwd HTTP/1.1
GET /..%252f..%252fetc/passwd HTTP/1.1
```

Ta config dit `root ./www`. Sans vérification, `buildPath` te donne `./www/../../../../etc/passwd`, `open()` réussit, et tu sers le fichier des utilisateurs de la machine. Depuis un simple navigateur.

**L'ordre est ce qui compte.** Le bug classique :
```cpp
// FAUX
if (uri.find("..") != std::string::npos) return error(403);
std::string path = root + percentDecode(uri);
```
`%2e%2e%2f` ne contient pas `..` — il passe le test. Puis le décodage le transforme en `../`. La vérification a été faite sur la mauvaise chaîne.

L'autre bug classique :
```cpp
// FAUX AUSSI
if (uri.find("..") != std::string::npos) return error(403);
```
`....//` ne contient pas `../` littéralement, mais après une normalisation naïve qui supprime `..`, il reste `..//`. Toute défense par filtrage de motif se contourne. **Ne filtre pas : canonicalise et vérifie.**

### La défense correcte

```cpp
bool safePath(const std::string& root, const std::string& uri, std::string& out)
{
    // 1. décoder D'ABORD, une seule fois
    std::string decoded;
    if (!percentDecode(uri, decoded))
        return false;                       // %ZZ, %A, %00 -> 400

    // 2. rejeter les octets nuls
    if (decoded.find('\0') != std::string::npos)
        return false;

    // 3. construire le chemin candidat
    std::string candidate = root + decoded;

    // 4. canonicaliser : realpath résout ., .., et les symlinks
    char resolved[PATH_MAX];
    char rootResolved[PATH_MAX];
    if (!realpath(root.c_str(), rootResolved))
        return false;
    if (!realpath(candidate.c_str(), resolved))
        return false;                       // n'existe pas -> 404

    // 5. LA vérification : le résultat est-il SOUS le root ?
    std::string r(rootResolved), p(resolved);
    if (p.compare(0, r.size(), r) != 0)
        return false;                       // 403
    if (p.size() > r.size() && p[r.size()] != '/')
        return false;                       // /var/wwwEVIL n'est pas sous /var/www

    out = p;
    return true;
}
```

Quatre points :

**① Décoder une seule fois.** Un double décodage rouvre la faille : `%252f` → `%2f` → `/`. Une passe, jamais deux.

**② `realpath` résout aussi les symlinks.** Si quelqu'un met `www/link -> /etc`, un contrôle purement textuel passe. `realpath` suit le lien et tu vois `/etc/passwd`, hors root. C'est la raison principale d'utiliser `realpath` plutôt qu'une normalisation maison.

**③ La comparaison de préfixe a besoin de la frontière.** `/var/www` est un préfixe de `/var/wwwEVIL`. Vérifie le `/` suivant. Même piège qu'au routing des `location`.

**④ `realpath` échoue si le chemin n'existe pas.** Pour un upload, tu dois canonicaliser le **dossier parent** puis vérifier le basename. Sinon tout upload est refusé.

`realpath` n'est pas dans la liste des fonctions autorisées du sujet. Deux options : le documenter comme fonction C standard (le sujet autorise les fonctions C et la liste vise les syscalls réseau), ou écrire ta propre canonicalisation par pile de segments. La seconde est plus sûre pour la note, mais **ne résout pas les symlinks** — mentionne-le dans le README, c'est un bon point de discussion.

Ta canonicalisation maison :
```cpp
std::string canonicalize(const std::string& p) {
    std::vector<std::string> stack;
    std::istringstream ss(p);
    std::string seg;
    while (std::getline(ss, seg, '/')) {
        if (seg.empty() || seg == ".") continue;
        if (seg == "..") { if (!stack.empty()) stack.pop_back(); continue; }
        stack.push_back(seg);
    }
    std::string out;
    for (size_t i = 0; i < stack.size(); ++i) out += "/" + stack[i];
    return out.empty() ? "/" : out;
}
```
Applique-la sur l'URI décodée, **avant** de concaténer au root. Un `..` en trop est absorbé par la pile vide. Tu ne peux plus sortir. Et c'est 100% du code que tu contrôles.

### L'octet nul

```
GET /index.html%00.jpg
```
Décodé : `index.html\0.jpg`. Si tu passes ça à `open()` via `c_str()`, le C s'arrête au `\0` et ouvre `index.html`. Ta vérification d'extension, elle, a vu `.jpg`.

C'est comme ça qu'on contournait les filtres d'upload pendant dix ans. **Rejette tout `%00` avec un 400**, sans discuter. Une ligne.

## 2. Le request smuggling

```
POST / HTTP/1.1
Host: x
Content-Length: 6
Transfer-Encoding: chunked

0

GET /admin HTTP/1.1
Host: x
```

Un proxy en amont croit `Transfer-Encoding` : il voit une requête qui finit au `0\r\n\r\n`, et transmet le reste comme une requête suivante. Ton serveur croit `Content-Length: 6` : il lit 6 octets et considère que `GET /admin` fait partie du body... ou pas, selon l'implémentation.

Résultat : le `GET /admin` devient une **requête fantôme** injectée dans la connexion de quelqu'un d'autre. Contournement d'authentification, empoisonnement de cache, vol de session. C'est une des classes de vulnérabilités les plus rentables du web moderne — un Black Hat entier a été construit dessus (James Kettle, 2019).

**La défense est une ligne** : `Content-Length` **et** `Transfer-Encoding` ensemble → **400**. C'est ce que dit la RFC 9112 §6.1, et ce n'est pas négociable.

Les variantes du même bug :
- Deux `Content-Length` avec des valeurs différentes → 400
- `Transfer-Encoding: chunked` obtenu par obfuscation (`Transfer-Encoding: xchunked`, `Transfer-Encoding:[tab]chunked`, `Transfer-Encoding\n: chunked`) → si ce n'est pas exactement `chunked` après trim, **501**, pas « on ignore »
- Espace avant le `:` → 400
- `Content-Length: 5 ` avec un espace final → trim, puis valide ; si ça ne parse pas en entier pur, 400

La règle générale : **sois strict**. En parsing de protocole, la tolérance est une faille. Le principe de robustesse (« be liberal in what you accept ») est aujourd'hui considéré comme une erreur historique précisément à cause de ça.

## 3. Slowloris

L'attaque la plus élégante du lot. Découverte par RSnake en 2009, toujours efficace.

```python
# ce que ferait un attaquant — implémente la défense, teste-la
socks = []
for i in range(500):
    s = socket.create_connection((HOST, PORT))
    s.sendall(b"GET / HTTP/1.1\r\n")
    socks.append(s)
while True:
    for s in socks:
        s.sendall(b"X-a: b\r\n")     # un header toutes les 15 secondes
    time.sleep(15)
```

Zéro bande passante. Zéro paquet malformé. Chaque requête est parfaitement légale — elle n'est juste jamais finie. Ton serveur garde 500 connexions ouvertes en attendant la fin des headers. À 1024 fds, tu ne peux plus accepter personne. Le serveur est mort, et un `tcpdump` ne montre rien d'anormal.

C'est ce qui a fait tomber des sites gouvernementaux iraniens en 2009 depuis un seul portable.

**Les défenses :**

```cpp
const int HEADER_TIMEOUT     = 15;    // secondes pour finir les headers
const int BODY_TIMEOUT       = 60;
const int IDLE_TIMEOUT       = 30;
const size_t MAX_HEADER_SIZE = 8192;
const size_t MAX_CONNS       = 512;   // < ulimit -n
```

1. **Timeout de headers** — la parade principale. Pas fini en 15 s → 408 + close.
2. **Taille max des headers** — 8 Ko, sinon je te noie sous des headers légitimes mais infinis.
3. **Limite de connexions** — au-delà, refuse. Un serveur qui refuse est mieux qu'un serveur mort.
4. **Limite par IP** (raffiné) — 20 connexions max par `REMOTE_ADDR`.

Ton test du fichier 09 en fait une version à 50 connexions. Le correcteur peut faire pire. Fais-le toi-même d'abord.

## 4. Le DoS par ressource

| Attaque | Défense |
|---|---|
| `Content-Length: 999999999999` | 413 **avant** d'allouer. Vérifie à la lecture des headers, pas après. |
| 100 000 chunks de 1 octet | Limite sur le **cumul**, pas par chunk |
| URI de 10 Mo | 414 dès que `_buf` dépasse 8 Ko sans CRLF |
| 50 000 headers de 10 octets | 431 sur le cumul des headers |
| CGI en boucle infinie | Timeout + `SIGKILL` |
| CGI qui produit 10 Go sur stdout | Limite la taille de sortie CGI → 502 |
| CGI qui forke à l'infini | `setrlimit(RLIMIT_NPROC)` dans l'enfant (hors scope, mais bonne réponse) |
| Zip bomb en upload | Tu ne décompresses rien. Non applicable. |

**Le principe** : chaque fois que le client contrôle une taille, tu dois avoir une limite, et elle doit être vérifiée **avant** l'allocation, pas après. « J'alloue 1 Go puis je réponds 413 » est un DoS.

La formulation qui marque en soutenance : *« tout ce que le client contrôle est un vecteur d'épuisement de ressource — RAM, fds, process, CPU, disque »*. Passe en revue chacun de ces cinq et dis quelle limite le protège.

## 5. L'injection de headers (CRLF injection)

Si tu recopies une valeur contrôlée par le client dans un header de réponse :

```
GET /old?next=%0d%0aSet-Cookie:%20admin=true
```
Décodé, le `next` contient `\r\nSet-Cookie: admin=true`. Si tu écris `Location: <next>` sans filtrer, ta réponse devient :
```
HTTP/1.1 302 Found
Location: 
Set-Cookie: admin=true
```
Tu viens de laisser le client injecter un header arbitraire. Avec un double CRLF, il injecte un **body entier** — c'est le response splitting, qui permet d'empoisonner un cache partagé.

**Défense** : tout ce qui va dans un header de réponse et vient du client est filtré de ses `\r` et `\n`. Sans exception. Concerné : `Location` (redirections, 201 après upload), et les headers que le CGI te rend.

**Le cas CGI est le plus vicieux** : le CGI est censé produire ses headers, tu les recopies. Mais si le CGI est buggé et recopie un paramètre GET dans un header... la faille est chez toi aussi. Valide chaque ligne de la sortie CGI : un nom de header au format token, `:`, une valeur sans caractères de contrôle. Sinon 502.

## 6. Le XSS de l'autoindex

```bash
touch 'www/<img src=x onerror=alert(document.cookie)>.txt'
```
C'est un nom de fichier parfaitement valide sous Linux. Ton autoindex le sert tel quel dans du HTML → XSS stocké dans ton propre serveur.

```cpp
std::string htmlEscape(const std::string& s) {
    std::string o;
    for (size_t i = 0; i < s.size(); ++i) {
        switch (s[i]) {
            case '&':  o += "&amp;";  break;
            case '<':  o += "&lt;";   break;
            case '>':  o += "&gt;";   break;
            case '"':  o += "&quot;"; break;
            case '\'': o += "&#39;";  break;
            default:   o += s[i];
        }
    }
    return o;
}
```

Et **deux échappements différents** : `htmlEscape` pour le texte affiché, `urlEncode` pour le `href`. Ce ne sont pas les mêmes règles. Confondre les deux est *la* cause historique des XSS : le bon échappement dépend du **contexte de destination**, pas de la source.

```html
<a href="<?= urlEncode(name) ?>"><?= htmlEscape(name) ?></a>
```

Ajoute aussi `X-Content-Type-Options: nosniff` sur toutes tes réponses. Une ligne : ça empêche le navigateur de deviner un `text/html` sur un fichier uploadé servi en `application/octet-stream`.

## 7. Les uploads

Le point le plus dangereux du projet : tu laisses un inconnu écrire sur ton disque.

| Risque | Défense |
|---|---|
| `filename="../../../etc/cron.d/evil"` | **Basename uniquement.** Jette tout ce qui contient `/` ou `\`. |
| `filename="../../../../.ssh/authorized_keys"` | Idem + canonicalisation du chemin final |
| `filename=""` ou `filename="."` | Rejette |
| `filename="shell.php"` dans un dossier CGI | **`upload_store` ne doit jamais être une location CGI** |
| Écraser un fichier existant | Refuse, ou renomme (`file_1.png`) |
| Remplir le disque | Limite le body **et** la taille totale du dossier |
| `filename` avec des `%00` | Rejette |
| Lien symbolique en destination | `O_NOFOLLOW` à l'open |

```cpp
std::string sanitizeFilename(const std::string& raw)
{
    // basename uniquement
    size_t slash = raw.find_last_of("/\\");
    std::string name = (slash == std::string::npos) ? raw : raw.substr(slash + 1);

    if (name.empty() || name == "." || name == "..") return "";
    if (name.find('\0') != std::string::npos)        return "";

    // liste blanche de caractères -- pas une liste noire
    std::string out;
    for (size_t i = 0; i < name.size(); ++i) {
        char c = name[i];
        if (std::isalnum(c) || c == '.' || c == '-' || c == '_')
            out += c;
        else
            out += '_';
    }
    if (out.size() > 255) out = out.substr(0, 255);
    if (out[0] == '.') out = "_" + out;   // pas de fichier caché
    return out;
}
```

**Liste blanche, pas liste noire.** Tu ne peux pas énumérer tous les caractères dangereux ; tu peux énumérer les sûrs. C'est vrai de toute validation d'entrée, et c'est le réflexe que le projet doit t'installer.

**Le pire scénario** : `upload_store ./www/cgi-bin` avec `cgi_ext .php`. J'uploade `shell.php`, je le demande, ton serveur l'exécute. RCE complète en deux requêtes. Ta config de démo ne doit **jamais** faire ça — et si le correcteur te le fait remarquer, tu dois savoir pourquoi c'est fatal.

## 8. Les risques CGI

**Shellshock** (CVE-2014-6271) : le CGI reçoit les headers HTTP dans son env. Un header `User-Agent: () { :;}; echo pwned` devenait `HTTP_USER_AGENT` dans l'environnement, et bash — lancé par le CGI — interprétait la définition de fonction au démarrage. RCE sur des millions de serveurs, dont la moitié du web de l'époque.

Ce n'est pas ta faille (c'était bash), mais ça illustre le modèle : **tout ce qui vient du client atterrit dans l'env du CGI**. Deux règles :
- Rien du client ne va dans le **nom** d'une variable. Seulement dans la valeur.
- Ne lance **jamais** un CGI via un shell. `execve("/bin/sh", "-c", cmd)` te donne une injection de commande gratuite. `execve(interpreter, {interpreter, script, NULL}, envp)` directement, jamais de shell entre les deux.

**Les autres :**
- Un CGI hérite de tous tes fds → `FD_CLOEXEC` (voir *07*)
- Le CGI tourne avec ton UID → il peut lire tout ce que tu peux lire. Un vrai serveur droppe les privilèges ; c'est hors scope, mais c'est la bonne réponse à « et si le script est malveillant ? »
- `access(script, X_OK)` avant de forker → 403 propre plutôt qu'un fork inutile

## 9. Le fuzzing

Tu veux valider ton parser pour de vrai :

```python
import random, socket
METHODS = [b"GET", b"POST", b"DELETE", b"", b"A"*1000, b"GET\x00"]
PATHS   = [b"/", b"/../../etc/passwd", b"/%00", b"/" + b"a"*10000, b"/\xff\xfe"]
VERS    = [b"HTTP/1.1", b"HTTP/9.9", b"", b"HTTP/1.1\r\nX: y"]

for i in range(50000):
    req = (random.choice(METHODS) + b" " + random.choice(PATHS) + b" " +
           random.choice(VERS) + b"\r\n")
    for _ in range(random.randint(0, 20)):
        req += bytes(random.choices(range(1,256), k=random.randint(1,50))) + b"\r\n"
    req += b"\r\n"
    try:
        s = socket.create_connection((HOST, PORT), timeout=0.5)
        s.sendall(req)
        s.recv(100)
        s.close()
    except: pass

# le serveur est-il encore vivant ?
assert socket.create_connection((HOST, PORT), timeout=2)
```

50 000 requêtes aléatoires. Le serveur doit être debout à la fin. Lance-le sous valgrind pour attraper les lectures hors bornes qui ne crashent pas mais corrompent.

Pour aller plus loin : **AFL++** ou **libFuzzer** sur ta fonction `feed()` isolée, sans réseau. C'est le vrai fuzzing, guidé par la couverture. Ce n'est pas exigé, c'est du travail, et c'est exactement le genre de chose qui fait qu'un correcteur se souvient de ta soutenance. Vu où tu vas, ça vaut le détour.

## 10. La checklist

À passer avant la soutenance :

- [ ] `/../../../../etc/passwd` → 403 ou 404, jamais le fichier
- [ ] `%2e%2e%2f` → pareil (décodage **avant** vérification)
- [ ] `%252f` → pareil (pas de double décodage)
- [ ] `....//` → pareil (canonicalisation, pas filtrage)
- [ ] `%00` dans une URI → 400
- [ ] Symlink `www/link -> /etc` → pas servi (ou documenté)
- [ ] CL + TE → 400
- [ ] Deux CL différents → 400
- [ ] `Transfer-Encoding: xchunked` → 501, pas ignoré
- [ ] Espace avant `:` → 400
- [ ] 500 connexions slowloris → le serveur répond toujours
- [ ] Timeout de headers effectif
- [ ] `Content-Length: 999999999999` → 413 sans allouer
- [ ] Fichier `<script>x</script>.txt` dans l'autoindex → échappé
- [ ] `href` de l'autoindex → URL-encodé
- [ ] `%0d%0a` dans une cible de redirection → filtré
- [ ] Upload `filename="../../x"` → basename ou rejet
- [ ] `upload_store` n'est pas une location CGI
- [ ] CGI lancé sans shell
- [ ] CGI en boucle infinie → 504 + SIGKILL, pas de zombie
- [ ] `X-Content-Type-Options: nosniff` sur toutes les réponses
- [ ] 50 000 requêtes fuzz → serveur debout

## 11. Le paragraphe README

Trois lignes qui changent une soutenance :

> **Security considerations.** Path traversal is prevented by decoding the URI once, rejecting null bytes, then canonicalizing the resolved path and verifying it stays under the configured root (with segment-boundary checking). Request smuggling is prevented by rejecting any request carrying both `Content-Length` and `Transfer-Encoding`, or duplicate `Content-Length` headers. Slowloris is mitigated by a 15-second header timeout, an 8 KB header size cap, and a connection limit. Directory listings HTML-escape filenames and URL-encode links. Uploaded filenames are reduced to their basename and filtered through a character allowlist. CGI processes are executed directly via `execve` — never through a shell — and are killed with `SIGKILL` after a 10-second timeout.

Aucun autre groupe n'aura ce paragraphe.

## 12. À retenir

- Décode **une fois**, puis canonicalise, puis vérifie le préfixe **avec frontière**.
- Liste blanche, jamais liste noire.
- CL + TE = 400. La tolérance en parsing de protocole est une faille.
- Timeout de headers = ta défense slowloris.
- Chaque taille contrôlée par le client a une limite, vérifiée **avant** l'allocation.
- Deux échappements différents : HTML pour le texte, URL pour les liens. Le contexte de destination décide.
- Filtre les CRLF de tout ce qui va dans un header de réponse.
- Upload : basename + liste blanche, jamais dans un dossier CGI.
- CGI : jamais de shell, jamais rien du client dans un nom de variable.
- Ton serveur doit survivre à 50 000 requêtes aléatoires.
