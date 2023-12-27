#ifndef _ANALYSE_H
#define _ANALYSE_H

/**
 * La fonction analyse_arg "découpe" la chaîne arg selon les espaces et
 * retourne un tableau de chaînes représentant les différents éléments
 * de arg.
 * @param arg la chaîne à découper.
 * @return un tableau de chaînes. Le tableau est terminé par NULL.
 * Le tableau est alloué dynamiquement, on doit donc le libérer
 * après utilisation à l'aide de la fonction dispose_arg.
 */
char **analyse_arg(const char arg[]);

/**
 * Permet de libérer la mémoire occupée par le tableau retourné par
 * analyse_arg.
 */
void dispose_arg(char *argv[]);

#endif // _ANALYSE_H
