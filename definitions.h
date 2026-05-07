#ifndef __DEFINITIONS_H
#define __DEFINITIONS_H

#define SEUIL_BATTERIE_FAIBLE 11000  // mv

//////////////////////////////////////////
// DEFINITIONS
//////////////////////////////////////////
typedef enum e_etatsPAV {
  ARRET, ENCOURS,
  VIDE=20, PLEIN, VIDAGE
} T_ETATSPAV;

typedef enum e_typeTrame {  // Types de réception de trame du superviseur
  BONJOUR,
  TT_VIDE=20,
  TT_PLEIN,
  TT_VIDAGE
} T_TYPETRAME;

#endif