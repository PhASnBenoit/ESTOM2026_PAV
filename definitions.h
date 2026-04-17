#ifndef __DEFINITIONS_H
#define __DEFINITIONS_H

#define SEUIL_BATTERIE_FAIBLE 6.8   // tension seuil batterie faible (V)

//////////////////////////////////////////
// DEFINITIONS
//////////////////////////////////////////
typedef enum e_etatsPAV {
  PLEIN, VIDE, VIDAGE
} T_ETATSPAV;

typedef enum e_typeTrame {  // Types de réception de trame du superviseur
  BONJOUR=1
} T_TYPETRAME;

#endif