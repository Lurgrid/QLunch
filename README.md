# GLunch - C Command Launcher

GLunch est un projet open source visant à réaliser un lanceur de commandes en utilisant le langage C. Il se compose de trois parties majeurs distinctes : une bibliothèque de files synchronisées, un programme lanceur de commandes qui est un daemon, et un programme client permettant de soumettre des demandes de lancement de commandes. Les autres modules présent, ne sont la qu'à titre d'outils du projet.

## Fonctionnalités

1. **Bibliothèque de files synchronisées**
   - Implémentation d'une file synchronisée permettant une communication entre le lanceur et ces possibles clients.

2. **Lanceur de commandes**
   - Utilisation de la file synchronisée pour récupérer et exécuter les commandes soumises par les clients.
   - Gestion concurrente des commandes pour assurer un fonctionnement robuste.
	- Le lanceur peut exécuter des commandes de la forme `cmd1 | cmd2 | ... | cmdn`.

3. **Client**
   - Possibilité de déposer des demandes de lancement de commandes dans la file synchronisée du lanceur.
   - Communication efficace avec le lanceur pour garantir un traitement rapide des demandes.

## Installation

1. **Clonage du dépôt**

```bash
git@github.com:Lurgrid/QLunch.git
```

2. **Compilation du lanceur et du client**

```bash
cd GLunch
make
```

## Utilisation

- Exécutez le lanceur de commandes :

```bash
cd ./server
./qlunch
```

Deux signaux sont fournie pour interagire avec le server.

 1. ```SIGTERM```, permet de terminer proprement le server. C'est à dire en libérant toutes ces ressources.
 2. ```SIGHUP```, permet de demander au server de recharger sont fichier de configuration. Ceci permet d'appliquer les possibles nouvelles valeurs présente dans ce fichier à la configuration actuelle du server.

&nbsp;

- Exécutez le client pour soumettre des commandes (cette execution dois forcement ce faire après avoir lancer le server) :

```bash
cd ./client
./qlunch_client
>>> :h
To get the help	:h
To quit	:q
>>> ls -l | wc -l
10
>>> :q
```

- Un fichier de configuration est mise a disposition. Sont chemin d'accès peut être modifier dans le fichier utilitie.h. Ce fichier permet en outre de définire un délai entre chaque éxécution de commandes par le serveur. 

Ce fichier est de la forme :

```
FIFO_NAME=qlunch
FIFO_LENGTH=2048
FIFO_TIME=0
#exemple de commentaire
```

## Licence

Ce projet est distribué sous la licence GPLv3. Consultez le fichier [LICENSE](LICENSE) pour plus de détails sur les conditions d'utilisation et de distribution.

---

> GitHub [@titusse3](https://github.com/titusse3) &nbsp;&middot;&nbsp;
> GitHub [@Lurgrid](https://github.com/Lurgrid) &nbsp;&middot;&nbsp;
