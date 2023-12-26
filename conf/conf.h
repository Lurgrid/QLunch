//  Partie interface du module conf

//  Ce module fourni des outils nécessaire à la gestion de fichier de
//    configuration.

#ifndef CONF__H
#define CONF__H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

//  SEPARATOR : séparateur entre une clef du fichier de configuartion et sa
//    valeur.
#define SEPARATOR '='

//  COMMENT : symbole signifiant que la ligne qui le suit est un commentaire.
#define COMMENT '#'

//  struct keyval_t keyval_t : type et nom de type d'un contrôleur regroupant
//    les informations nécessaire à la gestion d'un couple clef-valeur.
typedef struct keyval_t keyval_t;

//  process_t : enumaration des valeurs de retours possible de la fonctions
//    conf_process. Leur signification est documenter dans la spécification de
//    la fonction conf_process.
typedef enum {
  DONE,
  ERROR_ALLOC,
  ERROR_UNKNOWN,
  ERROR_PROCESS
} process_t;

//  conf_kv_init : Tente d'allouer les resources néssaires pour gérer un couple
//    clé-valeur. key, une chaine de caractère, représentant le nom de la clé.
//    val, la valeur obtenue après avoir appliquer la fonction hdl sur ce qui
//    suit le key et le SEPARATOR. err un pointeur de chaine de caractère qui
//    décrivant la possible erreur obtenue lors du traitement d'un fichier de
//    configuration.
//    Renvoie NULL en cas de dépassement de capacité. Renvoie sinon un pointeur
//    vers la structure associé a ce couple clé-valeur.
extern keyval_t *conf_kv_init(const char *key, void *val, int (*hdl)(void *val,
    const char *value, const char **err));

//  conf_kv_isclose : Renvoie si le couple clé-valeur pointé par kv a été déjà
//    rencontré ou non.
extern bool conf_kv_isclose(const keyval_t *kv);

//  conf_kv_isclose : Renvoie la clef associé à la ressources de gestion de la
//    clé-valeur pointé par kv.
extern const char *conf_kv_getkey(const keyval_t *kv);

//  conf_kv_dispose : Sans effet si *kv vaut NULL, sinon libère les ressources
//    allouées à la gestion du couple clé-valeur pointé par kv et met *kv à
//    NULL.
extern void conf_kv_dispose(keyval_t **kv);

//  conf_process : akv est un tableau de nmemb clé-valeur supporter lors de ce
//    traitement.
//    La fonction effectue le traitement suivant :
//    - si toutes les clef-valeur du fichier pointé par f ont été traiter sans
//        erreur alors renvoie DONE, *index vaut nmemb et *err pointe sur NULL
//    - si il y a eu une erreur d'allocation alors renvoie ERROR_ALLOC, la
//         valeur de *index est indéterminer et *err pointe sur NULL
//    - sinon, si lors d'un traitement d'un couple, clef-valeur:
//        .La clef n'est pas présente dans akv alors renvoie ERROR_UNKNOWN, la
//         valeur de *index est indéterminer et *err pointe sur NULL
//        .En cas d'erreur l'ors d'un traitement d'un couple clef-valeur par sa
//          fonction, alors renvoie ERROR_PROCESS, *index vaut l'indice du
//          couple clé-valeur générateur de l'erreur et *err pointe sur une
//          chaine de caractère descripteur de l'erreur.
extern process_t conf_process(keyval_t **akv, size_t nmemb, FILE *f,
  size_t *index, const char **err);

#endif
