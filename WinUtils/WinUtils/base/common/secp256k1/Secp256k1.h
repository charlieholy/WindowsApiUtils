#pragma once
#include "include\secp256k1.h"
class CSecp256k1
{
public:
	CSecp256k1();
	~CSecp256k1();
	secp256k1_context const* getCtx();
	void toPublic(const unsigned char * _secret);
};

