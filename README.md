# GLunch - C Command Launcher

GLunch est un projet open source visant à réaliser un lanceur de commandes en utilisant le langage C. Il se compose de trois parties distinctes : une bibliothèque de files synchronisées, un programme lanceur de commandes qui est un daemon, et un programme client permettant de soumettre des demandes de lancement de commandes.

## Fonctionnalités

1. **Bibliothèque de files synchronisées**
   - Implémentation d'une file synchronisée permettant une communication sécurisée entre le lanceur et les clients.

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
cd GLunch
```

2. **Compilation du lanceur**

```bash
cd ./server/
make
```

3. **Compilation du client**

```bash
cd ./client/
make
```

## Utilisation

- Exécutez le lanceur de commandes :

```bash
./qlunch
```

- Exécutez le client pour soumettre des commandes :

```bash
./qlunch_client
>>> :h
To get the help	:h
To quit	:q
>>> ls -l | wc -l
10
>>> :q
```

## Licence

Ce projet est distribué sous la licence GPLv3. Consultez le fichier [LICENSE](LICENSE) pour plus de détails sur les conditions d'utilisation et de distribution.

---

> GitHub [@titusse3](https://github.com/titusse3) &nbsp;&middot;&nbsp;
> GitHub [@Lurgrid](https://github.com/Lurgrid) &nbsp;&middot;&nbsp;
