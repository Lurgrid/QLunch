//  Partie interface du module conf

//  Ce module permet de mettre en place, une gestion de fichier de configuration
//    pour son utilisateur.

#ifndef CONF__H
#define CONF__H

#include <stdlib.h>
#include <stdio.h>

//  SEPARATOR : séparateur entre la clef et sa valeur.
#define SEPARATOR '='
//  COMMENT : préfix des commentaires
#define COMMENT '#'

//  struct keyval_t keyval_t : type et nom de type d'un contrôleur regroupant
//    les informations nécessaire à la gestion d'une clef-valeur.
typedef struct keyval_t keyval_t;

//  enumaration des valeurs de retours possible de la fonctions conf_process.
//    (chaque valeur est définie dans la spécfication de la fonction
//    conf_process)
enum {
  ERROR_ALLOC = -1,
  ERROR_UNKNOWN = -2
};

//  conf_kv_init : Tente d'allouer les resources néssaires pour gérer une
//    clef-valeur. key représente la valeur de la clef, val le pointeur vers la
//    ressource à possiblement modifier l'ors de la rencontre de la clef et hdl,
//    une fonction représentent l'actions à éffectuer l'ors que la clef est
//    rencontré, avec val le pointeur vers la ressource à possiblement modifier,
//    value la chaine de carractère associé à la clef et err un pointeur de
//    chaine de caractère qui change de valeur en cas d'erreur.
//    Renvoie NULL en cas de dépassement de capacité. Renvoie sinon un pointeur
//    vers la structure associé a cette clef-valeur.
extern keyval_t *conf_kv_init(const char *key, void *val, int (*hdl)(void *val,
    const char *value, const char **err));

//  conf_kv_isclose : Renvoie si la clef-valeur pointé par kv a été déjà
//    rencontré ou non.
extern bool conf_kv_isclose(const keyval_t *kv);

//  conf_kv_isclose : Renvoie la clef associé à la ressources de gestion de la
//    clef-valeur pointé par kv.
extern const char *conf_kv_getkey(const keyval_t *kv);

//  conf_kv_dispose : Sans effet si *kv vaut NULL, sinon libère les ressources
//    allouées à la gestion de la clef-valeur pointé par kv et met *kv à NULL.
extern void conf_kv_dispose(keyval_t **kv);

extern int conf_process(keyval_t **akv, size_t len, FILE *f, const char **err);

#endif
