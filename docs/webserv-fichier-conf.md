# Comprendre le fichier `.conf` de Webserv

*Cours débutant — comment lire et rédiger la configuration de ton serveur HTTP.*

---

## L'idée de départ

Un fichier `.conf` **n'a pas de format imposé**. C'est toi qui inventes sa grammaire.
Ce qui le contraint, ce n'est pas une norme externe : le fichier doit simplement
**répondre à l'avance à toutes les questions que ton serveur se posera quand une
requête arrive**.

> Écrire un `.conf`, c'est anticiper ces questions et préparer les réponses.
> Si tu comprends les questions, tu sais rédiger n'importe quelle config.

---

## Les deux niveaux : `server` et `location`

La config s'organise en **poupées russes**, et cette imbrication traduit une *portée* :

| Bloc | Répond à la question | Ce qu'on y met |
|------|----------------------|----------------|
| `server` | « **Où** est-ce que j'écoute ? » | ce qui concerne **tout le site** |
| `location` | « **Comment** je réponds selon le chemin ? » | ce qui dépend de **l'URL demandée** |

**Règle simple pour savoir où placer une directive :**
> « Est-ce que ça change selon l'URL demandée ? »
> Oui → dans une `location`. Non → dans le `server`.

---

## Le bloc `server`

Il dit **où** on écoute et donne les réglages globaux du site.

```nginx
server
{
    listen                  0.0.0.0:8080;
    server_name             webserv;
    client_max_body_size    10M;
    error_page 404          /errors/404.html;
    error_page 500          /errors/500.html;
    error_page 502          /errors/502.html;
    error_page 503          /errors/503.html;

    location /
    {
        root            ./www;
        index           index.html;
        allow_methods   GET;
    }
}
```

### Les directives du `server`

- **`listen 0.0.0.0:8080;`** → l'IP et le port sur lesquels écouter.
  C'est ça qui définit « un site ». Deux blocs `server` sur deux ports =
  deux sites servis par le même programme.

- **`server_name webserv;`** → le nom du site. Utile seulement si tu fais les
  *virtual hosts* (hors du périmètre obligatoire).

- **`client_max_body_size 10M;`** → taille maximale du corps d'une requête.
  Au-delà, le serveur répond **413**.

- **`error_page 404 /errors/404.html;`** → « si je dois renvoyer une erreur 404,
  affiche le fichier `/errors/404.html` ».
  On peut regrouper plusieurs codes sur une même page :
  `error_page 500 502 503 /errors/50x.html;`

---

## ⚠️ Le piège du point-virgule `;`

Le `;` marque **la fin d'une directive**. Sans lui, le parseur ne sait pas où
une directive s'arrête et où la suivante commence.

```nginx
error_page 404  /errors/404.html    ← ❌ manque le ;
error_page 404  /errors/404.html;   ← ✅ correct
```

Chaque ligne de directive se termine par `;`. Les blocs, eux, sont délimités
par des accolades `{ }`.

---

## Le bloc `location` — le vrai déclic

Oublie le mot « location ». Remplace-le mentalement par :

> **« Quand l'URL commence par … , fais ça. »**

```nginx
location /
{
    root            ./www;
    index           index.html;
    allow_methods   GET;
}
```

Ça se lit littéralement :
« Quand l'URL commence par `/` (donc n'importe quelle URL du site), va chercher
les fichiers dans `./www` ; si on me demande juste le dossier, sers `index.html` ;
et on n'a le droit que de lire (`GET`). »

### `root` — le lien URL ↔ fichier sur le disque

C'est **la directive la plus importante**. Le serveur colle l'URL derrière le
`root` pour trouver le vrai fichier :

```
URL demandée :    /photo.png
root :            ./www
fichier cherché : ./www/photo.png
```

```
URL demandée :    /css/style.css
root :            ./www
fichier cherché : ./www/css/style.css
```

C'est mécanique : **`root` + `URL` = fichier sur le disque.**

### `index` — le cas du dossier

Quand tu tapes juste `http://localhost:8080/`, l'URL est `/` → le serveur vise
le **dossier** `./www`, pas un fichier précis. Or un dossier ne s'affiche pas.
`index` dit : « dans ce cas, sers ce fichier-là ».

```
URL demandée :   /            (= le dossier ./www)
index :          index.html
fichier servi :  ./www/index.html
```

> C'est la fameuse page d'accueil : tu tapes l'adresse, et `index.html` s'affiche.

### `allow_methods` — ce qu'on a le droit de faire

- `GET` = lire / récupérer une ressource (l'action normale de visite).
- `POST` = envoyer des données (formulaires, upload).
- `DELETE` = supprimer une ressource.

```nginx
allow_methods GET;        # lecture seule
```

- `GET /`  → autorisé, le serveur sert la page.
- `POST /` → refusé, le serveur répond **405 Method Not Allowed**.

---

## Pourquoi plusieurs `location` ?

Parce que **tous les chemins ne se comportent pas pareil.**

```nginx
location /
{
    allow_methods   GET;          # site normal : on lit seulement
    root            ./www;
}

location /upload
{
    allow_methods   GET POST;     # ici on peut AUSSI envoyer des fichiers
    root            ./www;
}
```

Deux comportements différents → deux locations. La location `/` reste le
**filet de sécurité** : elle attrape tout ce qu'aucune location plus précise
ne prend.

---

## Une page n'est PAS une location

C'est **l'erreur classique du débutant** : croire qu'il faut une location par page.
Non. Une location = une **règle** qui s'applique à une *famille* d'URLs. Une page =
un simple **fichier** sur le disque, servi par une location existante.

Exemple : je veux deux pages, `webserv:8080/teams/fducrot` et
`webserv:8080/teams/ludebarn`.

**Je ne fais PAS une location par personne.** La mécanique `root + URL = fichier`
marche à n'importe quelle profondeur d'URL, donc ma `location /` couvre déjà tout :

```nginx
location /
{
    root    ./www;
    index   index.html;
}
```

```
URL : /teams/fducrot    →  ./www/teams/fducrot
URL : /teams/ludebarn   →  ./www/teams/ludebarn
```

Il me suffit de créer les dossiers correspondants sur le disque :

```
./www/
  index.html
  teams/
    fducrot/
      index.html
    ludebarn/
      index.html
```

Résultat : `/teams/fducrot/` sert `./www/teams/fducrot/index.html`, et pareil pour
`ludebarn`. **Une seule location, zéro config par personne.** Ajouter une personne =
ajouter un dossier, sans jamais toucher au `.conf`.

> Une `location` correspond à un **préfixe d'URL**, pas à une page précise.
> `/teams/fducrot` et `/teams/ludebarn` commencent tous les deux par `/`,
> donc `location /` les couvre **tous**, à toutes les profondeurs.

---

## Dans quel cas j'ai besoin d'une location EN PLUS ?

Une seule règle décide de tout :

> **Tu crées une location en plus quand une famille d'URLs doit se comporter
> *autrement* que le reste du site.**

Si le comportement est identique (comme les pages `teams` ci-dessus), ta
`location /` fait déjà le travail. Voici les cas où le comportement *diffère*,
donc où une location dédiée devient nécessaire.

### 1. Méthodes différentes

Le reste du site est en lecture seule, mais une zone doit accepter l'envoi.

```nginx
location / {
    allow_methods   GET;
}

location /upload {
    allow_methods   GET POST;      # ici on peut envoyer des fichiers
    root            ./www;
}
```

Sans ça, un POST sur `/upload` serait refusé (405) par la règle `GET` seule.

### 2. Dossier source différent

Une partie du site va chercher ses fichiers ailleurs sur le disque.

```nginx
location / {
    root    ./www;              # site normal
}

location /downloads {
    root    /var/files;         # ces URLs pointent vers un autre dossier
}
```

`/downloads/data.zip` → `/var/files/downloads/data.zip`, pas `./www/...`.

### 3. Une redirection

Un chemin doit renvoyer ailleurs au lieu de servir un fichier.

```nginx
location /vieux {
    return 301 /nouveau;        # tout /vieux redirige vers /nouveau
}
```

### 4. Activer le listing de dossier (autoindex)

Le site cache le contenu des dossiers, mais une zone doit l'afficher.

```nginx
location / {
    autoindex   off;            # on ne montre pas le contenu des dossiers
}

location /public {
    autoindex   on;             # ici, on affiche la liste des fichiers
    root        ./www;
}
```

### 5. Exécuter un CGI

Certaines URLs ne servent pas un fichier mais lancent un programme.

```nginx
location /cgi-bin {
    root        ./www;
    cgi_ext     .py;
    cgi_pass    /usr/bin/python3;   # les .py sont exécutés, pas affichés
}
```

### 6. Une limite de taille spécifique

Une zone d'upload doit accepter des fichiers plus gros que le reste.

```nginx
location /upload {
    client_max_body_size   50M;     # ici on autorise de gros fichiers
}
```

### Le test à te poser à chaque fois

> « Pour ce chemin, est-ce qu'**au moins une règle change** — méthode, dossier,
> redirection, autoindex, CGI, taille max ? »

- **Oui** → location dédiée, avec seulement les règles qui diffèrent.
- **Non** (juste des pages ou dossiers en plus, servis pareil) → **rien à faire**,
  `location /` s'en occupe.

---

## Le scénario complet, étape par étape

Tu tapes `http://localhost:8080/` dans le navigateur. Le serveur déroule :

1. **« L'URL est `/`. Quelle location ? »** → `location /` correspond.
2. **« Méthode `GET` ? »** → `allow_methods GET` l'autorise. ✅
3. **« L'URL `/` désigne le dossier `./www`. »** → c'est un dossier, pas un fichier.
4. **« J'ai `index index.html`. »** → je sers `./www/index.html`.
5. Le serveur lit ce fichier et te l'envoie. **La page s'affiche.**

> Chaque ligne de la config a servi à répondre à une étape.
> **Une config = les réponses préparées à l'avance aux questions du serveur.**

---

## La méthode pour rédiger une config à froid

1. **Combien de sites ?** (combien de couples `ip:port`) → autant de blocs `server`.
2. **Pour chaque site, quels chemins se comportent différemment ?**
   (racine, zone d'upload, zone CGI, redirections) → autant de blocs `location`.
3. **Pour chaque chemin, réponds au parcours d'une requête :**
   où sont les fichiers (`root`), quoi servir pour un dossier (`index`),
   quelles méthodes (`allow_methods`), etc.

Exemple de raisonnement à voix haute :
> « Un site sur le port 8080. La racine sert des pages HTML en GET seulement.
> Mais `/upload` accepte le POST. Et `/vieux` redirige vers `/nouveau`. »

Cette phrase se traduit **directement** en un `server` + trois `location`.

---

## Aide-mémoire des directives

| Directive | Niveau | Question à laquelle elle répond |
|-----------|--------|--------------------------------|
| `listen` | server | Sur quelle IP:port j'écoute ? |
| `server_name` | server | Quel nom pour ce site ? (virtual host) |
| `client_max_body_size` | server / location | Corps de requête trop gros ? → 413 |
| `error_page` | server | Quel fichier afficher pour telle erreur ? |
| `root` | location | Dans quel dossier sont les fichiers ? |
| `index` | location | Quoi servir quand on demande un dossier ? |
| `allow_methods` | location | GET / POST / DELETE autorisés ici ? |
| `autoindex` | location | Afficher la liste d'un dossier ? (`on`/`off`) |
| `return` | location | Rediriger ce chemin ailleurs ? |
| `cgi_ext` + `cgi_pass` | location | Exécuter un CGI selon l'extension ? |

---

## Point de vigilance : la règle du `root`

Le sujet précise : `/kapouet` rooté sur `/tmp/www` →
`/kapouet/pouic/toto` cherche `/tmp/www/pouic/toto`.

Autrement dit : **le préfixe de la location est retiré avant d'être collé au
`root`.** Ce comportement n'est pas dans le `.conf` — c'est ton *code* qui doit
l'appliquer en interprétant le `.conf`. Bien le comprendre t'évite d'attendre le
mauvais chemin.

---

## Rappels du sujet à garder en tête

- Syntaxe libre, mais **« no regex required »** : le CGI se déclenche par
  **extension** (ex. `.bla`, `.py`), pas par une regex `location ~ \.bla$`.
- Le serveur doit avoir des **pages d'erreur par défaut** si le `.conf` n'en
  fournit pas.
- Un crash = **note de 0**. Ton parseur doit gérer proprement les erreurs de
  config (accolade non fermée, directive inconnue, port invalide…) sans planter.
