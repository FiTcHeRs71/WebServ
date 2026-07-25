# Webserv — Setup des tickets GitHub (Issues + Project)

> Comment on gère les tickets à 3. Setup unique (~30-40 min à deux), puis workflow quotidien en §5.
> Le détail des tickets (DoD, dépendances) est dans [`webserv-backlog.md`](webserv-backlog.md).
> La méthode de travail est dans [`webserv-mode-demploi.md`](webserv-mode-demploi.md).

Vocabulaire : un **ticket** = une **Issue** GitHub (avec un numéro `#12`). Le tableau de suivi = un **Project** (colonnes Kanban).

---

## Étape 1 — Les labels (une fois)

Repo → onglet **Issues** → **Labels** → **New label**. Créer ces 7 :

| Label | Couleur suggérée |
|---|---|
| `mod:config` | bleu |
| `mod:net` | vert |
| `mod:http` | violet |
| `mod:cgi` | orange |
| `mod:bonus` | rose |
| `blocked` | rouge |
| `needs-review` | jaune |

## Étape 2 — Le Project (tableau Kanban)

1. Repo → onglet **Projects** → **New project** → modèle **Board**.
2. Nom : « Webserv ».
3. Colonnes (champ « Status ») à avoir : **Backlog · In progress · Needs review · Done**.
   (`+` en haut d'une colonne pour en ajouter une.)

## Étape 3 — Créer les tickets vite (l'astuce)

Ne pas créer 40 issues une par une. Dans le tableau du Project, colonne **Backlog**, cliquer
**`+ Add item`** et **taper juste le titre** → ça crée un « draft ». Enchaîner les 40 titres
(Entrée entre chaque). En ~5 min tout le backlog est visible.

Titres à enchaîner (copier/coller ligne par ligne) :

```
[S0-01] Squelette repo + Makefile
[S0-02] Figer les headers d'interface
[S0-03] conf/default.conf + conf/bad/*.conf
[S0-04] Convention Git (branches, PR, review)
[A-01] Tokenizer config
[A-02] Parse bloc server
[A-03] Parse bloc location
[A-04] Validation + erreurs explicites
[A-05] resolve() match préfixe le plus long
[A-06] Valeurs par défaut
[B-01] ListenSocket (non-bloquant, SO_REUSEADDR)
[B-02] Boucle poll() + accept()
[B-03] Connection inBuf/outBuf, recv/send partiel
[B-04] Déconnexions + cleanup, zéro fd leak
[B-05] Timeouts (408)
[B-06] SIGINT shutdown propre + SIGPIPE ignoré
[B-07] Enregistrer les fds CGI dans le poll
[C-01] Parser request line + headers (incrémental)
[C-02] Body Content-Length + 413
[C-03] Body chunked (dé-chunkage)
[C-04] Erreurs parsing 400/501/505
[C-05] ResponseBuilder + pages d'erreur
[C-06] GET statique + index + MIME
[C-07] Autoindex
[C-08] return / redirections 301-302
[C-09] Méthodes non autorisées 405 + Allow
[C-10] POST upload (multipart + raw)
[C-11] DELETE
[D-01] fork + execve + pipes non-bloquants
[D-02] Env meta-variables + chdir
[D-03] Body dé-chunké vers stdin CGI
[D-04] Parse sortie CGI
[D-05] waitpid + timeout CGI, zéro zombie
[E-01] Bonus cookies (Cookie / Set-Cookie)
[E-02] Bonus sessions
[E-03] Bonus plusieurs types de CGI
[T-01] Suite de tests Python (30+ cas)
[T-02] Stress test (siege/ab)
[T-03] Comparaison nginx
[T-04] Démo soutenance (site + CGI + confs)
[T-05] README.md conforme
[T-06] Passe valgrind + relecture croisée
```

## Étape 4 — Convertir en Issue au moment de prendre le ticket

Un « draft » n'a pas de numéro. Quand quelqu'un **démarre** un ticket :

1. Cliquer le draft → **Convert to issue** → choisir le repo → il gagne un numéro (`#23`).
2. Coller la **DoD depuis `webserv-backlog.md`** dans la description, ajouter le **label** (`mod:net`…), **s'assigner**.
3. Glisser en colonne **In progress**.

Pas besoin de tout convertir d'un coup — on convertit au fil de l'eau.

## Étape 5 — Le workflow quotidien

```
git checkout -b feat/b-03-connection-buffers   # une branche = un ticket
# ... code + commits ...
git push -u origin feat/b-03-connection-buffers
```

Puis sur GitHub → **Compare & pull request** → dans la description : **`Closes #23`** →
ouvrir la PR (colonne *Needs review*, label `needs-review`). Un équipier relit, teste la DoD,
approuve, **merge** → l'issue `#23` se ferme automatiquement et part en *Done*.

Le mot magique : **`Closes #23`** (ou `Fixes #23`) dans la PR ou un commit → fermeture auto au merge.

> ⚠️ Règle d'équipe : **aucun merge sans relecture d'un autre.** À la soutenance, chacun peut
> être interrogé sur n'importe quelle ligne, y compris celles des autres.

---

## Automatisation utile

- **Auto-Done** : Project → **Workflows** (menu `…` en haut à droite) → activer
  « Item closed → Set status to Done ». Ainsi `Closes #23` au merge range l'issue en Done tout seul.

## Rappel sur les bonus (Module E)

Les tickets `E-01/02/03` sont créés (label `mod:bonus`) mais restent en **Backlog**.
**Ne pas les convertir/démarrer** tant que le mandatory n'est pas parfait
(tester officiel `tests/testeur/tester` OK + valgrind propre + relecture). Le sujet n'évalue le bonus
que si l'obligatoire est 100 % complet.

## Variante : tout scripter avec `gh` (si un jour vous voulez)

Alternative à la création manuelle : `gh auth login` puis un script qui crée labels + issues
en boucle (`gh label create`, `gh issue create`). Non retenu ici (on fait manuel), mais possible
plus tard pour recréer un backlog rapidement.
