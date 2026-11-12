// SPDX-License-Identifier: GPL-2.0-only
#include <asm/cop2.h>
#include <asm/octeon/octeon.h>
#include <crypto/aes.h>
#include <crypto/internal/skcipher.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/unaligned.h>

#include "octeon-crypto.h"

struct octeon_aes_key {
	u64 words[4];
	u64 key_length;
};

static int octeon_aes_key_init(struct octeon_aes_key *ctx, const u8 *key,
			      unsigned int keylen)
{
	unsigned int i;

	if (keylen != AES_KEYSIZE_128 && keylen != AES_KEYSIZE_192 &&
	    keylen != AES_KEYSIZE_256)
		return -EINVAL;
	memset(ctx->words, 0, sizeof(ctx->words));
	for (i = 0; i < keylen / sizeof(u64); i++)
		ctx->words[i] = get_unaligned_be64(key + i * sizeof(u64));
	ctx->key_length = keylen / sizeof(u64) - 1;
	return 0;
}

static void octeon_cbc_blocks(const struct octeon_aes_key *ctx, u8 *iv,
			      const u8 *src, u8 *dst, unsigned int size,
			      bool encrypt)
{
	struct octeon_cop2_state saved;
	unsigned long flags;
	u64 chain0 = get_unaligned_be64(iv);
	u64 chain1 = get_unaligned_be64(iv + sizeof(u64));

	flags = octeon_crypto_enable(&saved);
	/* COP2 AES register selectors from the Cavium Octeon SDK. */
	asm volatile(
		"dmtc2 %0,0x0104\n\t"
		"dmtc2 %1,0x0105\n\t"
		"dmtc2 %2,0x0106\n\t"
		"dmtc2 %3,0x0107\n\t"
		"dmtc2 %4,0x0110\n\t"
		"dmtc2 %5,0x0102\n\t"
		"dmtc2 %6,0x0103"
		:: "d"(ctx->words[0]), "d"(ctx->words[1]), "d"(ctx->words[2]),
		   "d"(ctx->words[3]), "d"(ctx->key_length), "d"(chain0), "d"(chain1));

	for (; size; size -= AES_BLOCK_SIZE, src += AES_BLOCK_SIZE, dst += AES_BLOCK_SIZE) {
		u64 in0 = get_unaligned_be64(src);
		u64 in1 = get_unaligned_be64(src + sizeof(u64));
		u64 out0, out1;

		if (encrypt) {
			asm volatile(
				"dmtc2 %2,0x0108\n\tdmtc2 %3,0x3109\n\t"
				"dmfc2 %0,0x0100\n\tdmfc2 %1,0x0101"
				: "=&d"(out0), "=&d"(out1) : "d"(in0), "d"(in1));
			chain0 = out0;
			chain1 = out1;
		} else {
			asm volatile(
				"dmtc2 %2,0x010c\n\tdmtc2 %3,0x310d\n\t"
				"dmfc2 %0,0x0100\n\tdmfc2 %1,0x0101"
				: "=&d"(out0), "=&d"(out1) : "d"(in0), "d"(in1));
			chain0 = in0;
			chain1 = in1;
		}
		put_unaligned_be64(out0, dst);
		put_unaligned_be64(out1, dst + sizeof(u64));
	}
	octeon_crypto_disable(&saved, flags);

	put_unaligned_be64(chain0, iv);
	put_unaligned_be64(chain1, iv + sizeof(u64));
}

static int octeon_cbc_setkey(struct crypto_skcipher *tfm, const u8 *key,
			     unsigned int keylen)
{
	return octeon_aes_key_init(crypto_skcipher_ctx(tfm), key, keylen);
}

static int octeon_cbc_crypt(struct skcipher_request *req, bool encrypt)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct skcipher_walk walk;
	int err;

	if (!IS_ALIGNED(req->cryptlen, AES_BLOCK_SIZE))
		return -EINVAL;
	err = skcipher_walk_virt(&walk, req, false);
	while (!err && walk.nbytes) {
		unsigned int size = round_down(min(walk.nbytes, 4096U), AES_BLOCK_SIZE);

		octeon_cbc_blocks(crypto_skcipher_ctx(tfm), walk.iv,
				  walk.src.virt.addr, walk.dst.virt.addr, size, encrypt);
		err = skcipher_walk_done(&walk, walk.nbytes - size);
	}
	return err;
}

static int octeon_cbc_encrypt(struct skcipher_request *req)
{
	return octeon_cbc_crypt(req, true);
}

static int octeon_cbc_decrypt(struct skcipher_request *req)
{
	return octeon_cbc_crypt(req, false);
}

static struct skcipher_alg octeon_cbc_alg = {
	.base.cra_name = "cbc(aes)",
	.base.cra_driver_name = "cbc-aes-octeon",
	.base.cra_priority = OCTEON_CR_OPCODE_PRIORITY,
	.base.cra_blocksize = AES_BLOCK_SIZE,
	.base.cra_ctxsize = sizeof(struct octeon_aes_key),
	.base.cra_module = THIS_MODULE,
	.min_keysize = AES_MIN_KEY_SIZE,
	.max_keysize = AES_MAX_KEY_SIZE,
	.ivsize = AES_BLOCK_SIZE,
	.setkey = octeon_cbc_setkey,
	.encrypt = octeon_cbc_encrypt,
	.decrypt = octeon_cbc_decrypt,
};

/* NIST SP 800-38A F.2.1, F.2.3 and F.2.5. Test IV handoff between chunks. */
static int __init octeon_cbc_selftest(void)
{
	static const struct {
		const char *key;
		const char *ciphertext;
		unsigned int keylen;
	} vectors[] = {
		{
			"2b7e151628aed2a6abf7158809cf4f3c",
			"7649abac8119b246cee98e9b12e9197d5086cb9b507219ee95db113a917678b2",
			AES_KEYSIZE_128
		},
		{
			"8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b",
			"4f021db243bc633d7178183a9fa071e8b4d9ada9ad7dedf4e5e738763f69145a",
			AES_KEYSIZE_192
		},
		{
			"603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
			"f58c4c04d6e5f1ba779eabfb5f7bfbd69cfc4e967edb808d679f777bc6702c7d",
			AES_KEYSIZE_256
		},
	};
	struct octeon_aes_key ctx;
	u8 iv[AES_BLOCK_SIZE], initial_iv[AES_BLOCK_SIZE], key[AES_MAX_KEY_SIZE];
	u8 plain[2 * AES_BLOCK_SIZE], expected[sizeof(plain)], out[sizeof(plain)];
	unsigned int i;

	if (hex2bin(initial_iv, "000102030405060708090a0b0c0d0e0f", sizeof(initial_iv)) ||
	    hex2bin(plain, "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51",
		    sizeof(plain)))
		return -EINVAL;
	for (i = 0; i < ARRAY_SIZE(vectors); i++) {
		if (hex2bin(key, vectors[i].key, vectors[i].keylen) ||
		    hex2bin(expected, vectors[i].ciphertext, sizeof(expected)) ||
		    octeon_aes_key_init(&ctx, key, vectors[i].keylen))
			return -EINVAL;

		memcpy(iv, initial_iv, sizeof(iv));
		octeon_cbc_blocks(&ctx, iv, plain, out, AES_BLOCK_SIZE, true);
		octeon_cbc_blocks(&ctx, iv, plain + AES_BLOCK_SIZE, out + AES_BLOCK_SIZE,
				  AES_BLOCK_SIZE, true);
		if (memcmp(out, expected, sizeof(out)) ||
		    memcmp(iv, expected + AES_BLOCK_SIZE, sizeof(iv)))
			return -EIO;

		memcpy(iv, initial_iv, sizeof(iv));
		octeon_cbc_blocks(&ctx, iv, out, out, sizeof(out), false);
		if (memcmp(out, plain, sizeof(out)) ||
		    memcmp(iv, expected + AES_BLOCK_SIZE, sizeof(iv)))
			return -EIO;
	}
	return 0;
}

static int __init octeon_cbc_init(void)
{
	int err;

	if (!IS_ENABLED(CONFIG_CPU_BIG_ENDIAN) || !octeon_has_crypto())
		return -ENODEV;
	err = octeon_cbc_selftest();
	if (err) {
		pr_err("octeon-aes-cbc: AES self-test failed (%d)\n", err);
		return err;
	}
	err = crypto_register_skcipher(&octeon_cbc_alg);
	if (!err)
		pr_info("octeon-aes-cbc: AES-128/192/256 self-tests passed, registered cbc-aes-octeon\n");
	return err;
}

static void __exit octeon_cbc_exit(void)
{
	crypto_unregister_skcipher(&octeon_cbc_alg);
}

module_init(octeon_cbc_init);
module_exit(octeon_cbc_exit);
MODULE_DESCRIPTION("Octeon COP2 AES-CBC acceleration");
MODULE_LICENSE("GPL");
MODULE_ALIAS_CRYPTO("cbc-aes-octeon");
