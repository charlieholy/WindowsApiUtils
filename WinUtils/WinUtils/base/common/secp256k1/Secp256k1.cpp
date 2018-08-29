#include "Secp256k1.h"

#include <array>
#include <memory>



CSecp256k1::CSecp256k1()
{
}


CSecp256k1::~CSecp256k1()
{
}


secp256k1_context const* CSecp256k1::getCtx()
{
	static std::unique_ptr<secp256k1_context, decltype(&secp256k1_context_destroy)> s_ctx{
		secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY),
		&secp256k1_context_destroy
	};
	return s_ctx.get();
}
using byte = unsigned char;
void CSecp256k1::toPublic(const unsigned char * _secret)
{
	auto* ctx = getCtx();
	secp256k1_pubkey rawPubkey;
	// Creation will fail if the secret key is invalid.
	if (!secp256k1_ec_pubkey_create(ctx, &rawPubkey, _secret))
		return;
	std::array<byte, 65> serializedPubkey;
	size_t serializedPubkeySize = serializedPubkey.size();
	secp256k1_ec_pubkey_serialize(
		ctx, serializedPubkey.data(), &serializedPubkeySize,
		&rawPubkey, SECP256K1_EC_UNCOMPRESSED
	);
	printf("over");
}