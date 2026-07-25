# Convention de commits — Webserv

*Format d'écriture des messages de commit pour l'équipe. Basé sur [Conventional Commits](https://www.conventionalcommits.org), adapté à nos tickets (`A-01`, `B-03`…) et à nos modules.*

> Objectif : un historique lisible où on retrouve d'un coup d'œil **quel type** de changement touche **quel module**, et **quel ticket** il fait avancer. Chacun doit pouvoir relire l'historique des autres à la soutenance.

---

## Le format

```
<type>(<scope>): <description> (<ID-ticket>)
```

Exemple minimal :

```
feat(config): tokenizer du fichier .conf (A-01)
```

- **tout en minuscules** sauf les noms propres (`GET`, `POLLIN`…) ;
- **description à l'impératif présent** : « ajoute », « corrige », pas « ajouté » ni « ajout de » ;
- **pas de point final** ;
- **sujet court** : viser ≤ 50 caractères, 72 au grand maximum.

---

## Les `type` autorisés

| Type | Quand l'utiliser |
|---|---|
| `feat` | nouvelle fonctionnalité (le gros du projet : parseur, sockets, handlers…) |
| `fix` | correction de bug |
| `refactor` | réécriture sans changer le comportement (renommage, découpage, forme canonique…) |
| `test` | ajout/modif de tests (`tests/run.py`, mocks, cas `conf/bad/`…) |
| `docs` | documentation seule (`docs/`, commentaires Doxygen, README) |
| `chore` | tuyauterie : Makefile, `.gitignore`, scripts, arbo, dépendances |

> Pas de `style`/`perf`/`build` séparés : `chore` et `refactor` suffisent pour ce projet.

---

## Les `scope` autorisés

Le scope = le **module** touché (voir [`webserv-backlog.md`](../webserv-backlog.md)). Il est **optionnel** pour les changements transverses.

| Scope | Périmètre |
|---|---|
| `config` | Module A — parseur `.conf`, `ServerConfig`, `LocationConfig`, `Resolve()` |
| `net` | Module B — sockets, `poll()`, `Connection`, buffers, timeouts |
| `http` | Module C — `Request`, `Response`, router, handlers, pages d'erreur |
| `cgi` | Module D — `CgiProcess`, fork/execve/pipes |
| `repo` | transverse — Makefile, arbo, CI, scripts (souvent avec `chore`) |
| *(aucun)* | doc générale, changements multi-modules |

---

## L'ID de ticket

Entre parenthèses **à la fin du sujet**, il relie le commit à son ticket du backlog et à la branche (`feat/a-01-...`).

```
feat(config): parse le bloc server (A-02)
fix(net): arme POLLOUT seulement si outBuf non vide (B-03)
```

- **Obligatoire** dès qu'un commit fait avancer un ticket A/B/C/D/E.
- **Omis** pour un changement hors ticket (`docs`, `chore` ponctuel).

> La **fermeture** du ticket ne se fait pas dans le commit mais dans la **PR**, via `Closes #NN` (numéro d'issue GitHub). L'ID (`A-02`) et le numéro d'issue (`#12`) sont deux choses distinctes — cf. [`github-setup.md`](../github-setup.md).

---

## Le corps du message (optionnel)

Pour expliquer le **pourquoi** quand ce n'est pas évident. Séparé du sujet par une ligne vide, lignes ≤ 72 caractères.

```
fix(http): rejette Content-Length + Transfer-Encoding ensemble (C-04)

La RFC 7230 §3.3.3 interdit les deux en même temps (risque de
request smuggling). On répond 400 au lieu de deviner lequel suivre.
```

---

## Exemples ✅ / ❌

| ❌ Évite | ✅ Préfère |
|---|---|
| `Adding socket class` | `feat(net): ajoute la classe ListenSocket (B-01)` |
| `Update Makefile` | `chore(repo): lie chaque .cpp à son header dans le Makefile` |
| `fix bug` | `fix(config): rejette un port hors borne 0-65535 (A-04)` |
| `WIP` | *(ne pas committer de WIP sur une branche relue — squash avant la PR)* |
| `feat(config): Tokenizer Du .conf.` | `feat(config): tokenizer du .conf (A-01)` |
| `Finish comments Class` | `docs(cgi): commente les méthodes de CgiProcess` |

---

## Cohérence branche ↔ commit ↔ PR

```
branche :  feat/a-01-tokenizer-config
commits :  feat(config): tokenizer du .conf (A-01)
           test(config): dump des tokens sur les 21 confs (A-01)
PR      :  [A-01] Tokenizer du fichier .conf
           corps → Closes #12
```

Le préfixe de branche (`feat/`) et le `type` du commit se répondent ; l'ID (`A-01`) est le fil rouge de bout en bout.

---

_PR template : [`pull_request_template.md`](pull_request_template.md) · Workflow complet : [`../webserv-mode-demploi.md`](../webserv-mode-demploi.md)_
