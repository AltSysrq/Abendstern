/**
 * @file
 * @author Jason Lingle
 * @brief Defines symbolic constants for the 8-bit ship pallet.
 */

/*
 * ship_pallet.hxx
 *
 *  Created on: 26.01.2011
 *      Author: jason
 */

#ifndef SHIP_PALLET_HXX_
#define SHIP_PALLET_HXX_

#define P_TRANSPARENT   0x00
#define P_BLACK         0x01
#define P_BODY_BEGIN    0x02
#define P_BODY_END      0x7F
#define P_BODY_SZ       (P_BODY_END-P_BODY_BEGIN+1)
#define P_DYNAMIC_BEGIN 0x80
#define P_DYNAMIC_END   0xFF
#define P_DYNAMIC_SZ    (P_DYNAMIC_END-P_DYNAMIC_BEGIN+1)
#define P_FUSION_POW    0x80
#define P_FISSION_POW   0x81
#define P_ANTIM_POW     0x82
#define P_CAPACITOR     0x83
#define P_HEATSINK      0x84
#define P_BUSSARD0      0x90
#define P_BUSSARD1      0x91
#define P_BUSSARD2      0x92
#define P_BUSSARD3      0x93
#define P_SHIELD0       0x94
#define P_SHIELD1       0x95
#define P_SHIELD2       0x96
#define P_SHIELD3       0x97
#define P_SHIELD4       0x98
#define P_SHIELD5       0x99
#define P_G_CYANDARK0   0xC0
#define P_G_CYANDARK1   0xC1
#define P_G_CYANDARK2   0xC2
#define P_G_CYANDARK3   0xC3
#define P_G_BLUEDARK0   0xC4
#define P_G_BLUEDARK1   0xC5
#define P_G_BLUEDARK2   0xC6
#define P_G_BLUEDARK3   0xC7
#define P_G_YELLORNG0   0xC8
#define P_G_YELLORNG1   0xC9
#define P_G_YELLORNG2   0xCA
#define P_G_YELLORNG3   0xCB
#define P_G_GREYLGHT0   0xCC
#define P_G_GREYLGHT1   0xCD
#define P_G_GREYLGHT2   0xCE
#define P_G_GREYLGHT3   0xCF
#define P_GREEN         0xFC
#define P_DARKYELLOW    0xFD
#define P_DARKBLUECYAN  0xFE
#define P_WHITE         0xFF

#define P_IX2F(ix) ((ix)/256.0f)

#endif /* SHIP_PALLET_HXX_ */
