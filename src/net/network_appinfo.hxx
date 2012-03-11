#ifndef NETWORK_APPINFO_HXX_
#define NETWORK_APPINFO_HXX_

/**
 * @file
 * @author Jason Lingle
 * @date 2012.03.11
 * @brief Contains declarations of the values to send in STX packets.
 */

/**
 * The contents of the application field of STX packets.
 */
static const unsigned char applicationName[] = "Abendstern";
/**
 * The hash of the network protocol description used to ensure compatibility,
 * sent in the protocol hash field of STX packets.
 */
//Defined in src/net/xxx/xnetobj.cxx
extern const unsigned char protocolHash[32];

#endif /* NETWORK_APPINFO_HXX_ */
