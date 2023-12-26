//  Partie interface du module shm_fifo

//  Ce module fournie des outils permettant la création et la gestion d'une file
//    synchroniser de pid dans un segment de mémoire partager.

#ifndef SHM_FIFO__H
#define SHM_FIFO__H

#include <sys/types.h>
#include <limits.h>

//  struct fifo_t fifo_t : type et nom de type d'un contrôleur regroupant
//    les informations nécessaire à la gestion d'une file synchroniser.
typedef struct fifo_t fifo_t;

//  shm_fifo_init : fonction créant et ouvrant une file synchroniser de pid_t.
//    De nom name et de longueur length. La chaine name ne doit pas contenir de
//    caractère '/'. De plus cette chaine ne peut pas exéder la longeur
//    NAME_MAX - 1 (définie dans limits.h) sinon le resultat est indéterminer.
//    Renvoie NULL en cas d'erreur. Sinon renvoie un pointeur vers la file.
extern fifo_t *shm_fifo_init(const char *name, unsigned int length);

//  shm_fifo_init : fontion ouvrant une file synchroniser d'element de type
//    pid_t. De nom name et de longueur length.
//    Renvoie NULL en cas d'erreur. Sinon renvoie un pointeur vers la file.
extern fifo_t *shm_fifo_open(const char *name, unsigned int length);

//  shm_fifo_enqueue : fonction enfilant le pid p dans la file pointé par f.
//    Cette appelle est bloquant si la file est plein ou qu'un accés concurent
//    est en cours. La chaine name ne peut pas contenir de caractère '/'.
//    Renvoie -1 en cas d'erreur, une valeur nulle sinon.
extern int shm_fifo_enqueue(fifo_t *f, pid_t p);

//  shm_fifo_dequeue : fonction defilant un pid de la file pointé par f.
//    Cette appelle est bloquant si la file est vide ou qu'un accés concurent
//    est en cours.
//    Renvoie (pid_t) -1 en cas d'erreur, le pid défiler sinon.
extern pid_t shm_fifo_dequeue(fifo_t *f);

//  shm_fifo_dispose : Sans effet si *f vaut NULL, sinon libère les ressources
//    allouées à la gestion de la file synchroniser pointé par *f et met *f à
//    NULL.
//    Renvoie -1 en cas d'erreur, une valeur nulle sinon.
extern int shm_fifo_dispose(fifo_t **f);

//  shm_fifo_unlink : Fonction indiquant la fermeture de la file. Ce qui
//    implique qu'aucune nouvelle ouverture de la file ne peut être effectuer.
//    Mais aussi qu'au moment au plus aucun utilisateur n'a la file ouverte,
//    celle-ci se voit supprimer. La chaine name ne doit pas contenir de
//    caractère '/'. De plus cette chaine ne peut pas exéder la longeur
//    NAME_MAX - 1 (définie dans limits.h) sinon le resultat est indéterminer.
//    Renvoie -1 en cas d'erreur, une valeur nulle sinon.
extern int shm_fifo_unlink(const char *name);

#endif
