#ifndef UTIL__H
#define UTIL__H

#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#define _XOPEN_SOURCE 500

//  Certain des fonctions présente dans ce module ne sont pas spécialement
//    construite de facon a pouvoir etre reutliser. En effet, un bon nombre
//    d'entre elle produisent des effets de bords. En la présence de message
//    d'erreur affichier à l'aide de fonction tel que fprintf.
//    Il est donc bienvenue d'utiliser judisieusement ce module en connaissance
//    des divers point faible de celui-ci.

#include <stdlib.h>
#include <limits.h>

//  CONFIG_NAME : Nom du fichier de configuration.
#define CONFIG_NAME "../qlunch.conf"

//  ARRAY_LENGTH(arr) : Fonction renvoiyant la longeure du tableau arr.
#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof *(arr))

//  ERR_MEMORY : message correspondant à une erreur de mémoire.
#define ERR_MEMORY "Memory error (generally not enougth memory).%s\n"

//  ERR_FILE : message correspondant à une erreur de fichier.
#define ERR_FILE "File not found or not readeble.%s\n"

//  ERR_KEY : message correspondant à une erreur de clé.
#define ERR_KEY "File contain an non-defined key.%s\n"

// conf_t : structure regroupant les information nécéssaire à la configuration
//  du server.
typedef struct {
	char fifo_name[NAME_MAX];
	unsigned int fifo_length;
	unsigned int time;
} conf_t;

//  array_map : apllique à tous les éléments du tableau pointé par base de
//    nmemb élément de taille size la fonction f. Si la fontion f renvoie une
//    valeur non nulle alors le traitement est stoppé.
//    Renvoie 0 en cas de succés. Sinon -1 est renvoyer si f a un moment
//    donner renvoier une valeurs différente de 0.
extern int array_map(void *base, size_t nmemb, size_t size, int (*f)(void *v));

//  get_valid_name : Renvoie une valeur nulle dans le cas v représente un nom
//    de fichier valide et met dans n une copie de v. Un nom est valide s'il
//    ne déppasse pas NAME_MAX(définie dans limits.h) et qu'il ne contiennent
//    pas de le caractère '/'. Renvoie une valeur différente de 0 dans le cas
//    contraire et met *err à un texte d'écrivant l'erreur.
extern int get_valid_name(char *n, const char *v, const char **err);

//  un_from_char : Fonction de conversion de la chaine pointé par v en un
//    unsigned int.
//    Renvoie 0 en cas de succé et *s vaut la conversion de v en unsigned int.
//    Sinon renvoie une valeur non nulle, la valeur de *s n'est pas spécifier
//    et *err représente un texte d'écrivant l'erreur.
extern int un_from_char(unsigned int *s, const char *v, const char **err);

//  write_char_buff : Fonction qui écrit dans le descripteur de fichier supposer
//    ouvert desc, les length caractère présent dans buffer.
//    Renvoie 0 en cas de succé, une valeur non nul sinon.
extern int write_char_buff(int desc, const char *buffer, size_t length);

// process_conf_file : Fonction lisant le fichier de chemin d'accés CONFIG_NAME.
//   Les informations néccéssaire de ce fichier seront alors mise dans *conf.
//   Renvoie 0 en cas de succés, une valeur non nul sinon.
extern int process_conf_file(conf_t *conf);

//  is_prefixe : Renvoie 0 si la chaine ponité par s1 est suffixe propre de s2.
//    Une valeur strictement supérieur à 0 dans le cas ou s1 est suffixe de s2.
//    Sinon renvoie une valeur strictemetn négatife.
extern int is_prefixe(const char *s1, const char *s2);

#endif
