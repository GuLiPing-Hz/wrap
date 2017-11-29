// Copyright (c) 2014-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMPAT_ENDIAN_H
#define BITCOIN_COMPAT_ENDIAN_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <stdint.h>
#include "../wrap/funcs.h"

template<class T>
inline T htobe(T v){
	doswap(doendian(OP_BIGENDIAN), &v, sizeof(v));
	return v;
}

template<class T>
inline T htole(T v){
	doswap(doendian(OP_LITTLEENDIAN), &v, sizeof(v));
	return v;
}

template<class T>
inline T betoh(T v){
	doswap(doendian(OP_BIGENDIAN), &v, sizeof(v));
	return v;
}

template<class T>
inline T letoh(T v){
	doswap(doendian(OP_LITTLEENDIAN), &v, sizeof(v));
	return v;
}

inline uint16_t htobe16(uint16_t host_16bits)
{
    //return bswap_16(host_16bits);
	return htobe<uint16_t>(host_16bits);
}
inline uint16_t htole16(uint16_t host_16bits)
{
    //return host_16bits;
	return htole<uint16_t>(host_16bits);
}
inline uint16_t be16toh(uint16_t big_endian_16bits)
{
    //return bswap_16(big_endian_16bits);
	return betoh<uint16_t>(big_endian_16bits);
}
inline uint16_t le16toh(uint16_t little_endian_16bits)
{
    //return little_endian_16bits;
	return letoh<uint16_t>(little_endian_16bits);
}
inline uint32_t htobe32(uint32_t host_32bits)
{
    //return bswap_32(host_32bits);
	return htobe<uint32_t>(host_32bits);
}
inline uint32_t htole32(uint32_t host_32bits)
{
    //return host_32bits;
	return htole<uint32_t>(host_32bits);
}
inline uint32_t be32toh(uint32_t big_endian_32bits)
{
    //return bswap_32(big_endian_32bits);
	return betoh<uint32_t>(big_endian_32bits);
}
inline uint32_t le32toh(uint32_t little_endian_32bits)
{
    //return little_endian_32bits;
	return letoh<uint32_t>(little_endian_32bits);
}
inline uint64_t htobe64(uint64_t host_64bits)
{
    //return bswap_64(host_64bits);
	return htobe<uint64_t>(host_64bits);
}
inline uint64_t htole64(uint64_t host_64bits)
{
    //return host_64bits;
	return htole<uint64_t>(host_64bits);
}
inline uint64_t be64toh(uint64_t big_endian_64bits)
{
    //return bswap_64(big_endian_64bits);
	return betoh<uint64_t>(big_endian_64bits);
}
inline uint64_t le64toh(uint64_t little_endian_64bits)
{
    //return little_endian_64bits;
	return letoh<uint64_t>(little_endian_64bits);
}

#endif // BITCOIN_COMPAT_ENDIAN_H
