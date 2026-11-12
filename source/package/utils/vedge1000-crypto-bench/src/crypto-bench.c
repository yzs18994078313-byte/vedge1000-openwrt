#define _POSIX_C_SOURCE 200809L

#include <linux/if_alg.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#if defined(__mips__) && __SIZEOF_POINTER__ == 8 && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define HAVE_OCTEON_COP2 1
#else
#define HAVE_OCTEON_COP2 0
#endif

#define KIB 1024
#define MIB (1024 * KIB)
#define BLOCK_SIZE 16

struct aes_case {
	unsigned int bits;
	const EVP_CIPHER *(*cipher)(void);
	const char *key;
	const char *expected;
};

/* The first two blocks of the NIST SP 800-38A F.2.1 and F.2.5 examples. */
static const struct aes_case cases[] = {
	{
		128, EVP_aes_128_cbc, "2b7e151628aed2a6abf7158809cf4f3c",
		"7649abac8119b246cee98e9b12e9197d5086cb9b507219ee95db113a917678b2"
	},
	{
		256, EVP_aes_256_cbc,
		"603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
		"f58c4c04d6e5f1ba779eabfb5f7bfbd69cfc4e967edb808d679f777bc6702c7d"
	}
};

static void openssl_error(void)
{
	fputs("OpenSSL AES-CBC failed\n", stderr);
	ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
}

static EVP_CIPHER_CTX *software_context(const struct aes_case *test,
				      const unsigned char *key, bool encrypt)
{
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

	if (!ctx || !EVP_CipherInit_ex(ctx, test->cipher(), NULL, key, NULL, encrypt)
	    || !EVP_CIPHER_CTX_set_padding(ctx, 0))
		openssl_error();
	return ctx;
}

static void software_cbc(EVP_CIPHER_CTX *ctx, const unsigned char *iv,
			 const unsigned char *in, unsigned char *out, size_t size)
{
	int written, final;

	if (!EVP_CipherInit_ex(ctx, NULL, NULL, NULL, iv, -1)
	    || !EVP_CipherUpdate(ctx, out, &written, in, (int)size)
	    || !EVP_CipherFinal_ex(ctx, out + written, &final)
	    || (size_t)(written + final) != size)
		openssl_error();
}

#if HAVE_OCTEON_COP2
static void illegal_instruction(int signal_number)
{
	static const char message[] = "Octeon COP2 AES instruction is unavailable.\n";

	(void)signal_number;
	(void)write(STDERR_FILENO, message, sizeof(message) - 1);
	_exit(EXIT_FAILURE);
}

/* The existing Octeon SHA driver only registers on a crypto-capable CPU. */
static void check_hardware(void)
{
	struct sockaddr_alg address = {
		.salg_family = AF_ALG, .salg_type = "hash", .salg_name = "octeon-sha1"
	};
	struct sigaction action = { .sa_handler = illegal_instruction };
	int fd = socket(AF_ALG, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);

	if (fd < 0 || bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("Octeon hardware probe (octeon-sha1)");
		exit(EXIT_FAILURE);
	}
	close(fd);
	sigemptyset(&action.sa_mask);
	if (sigaction(SIGILL, &action, NULL) < 0) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
}

/* AES register selectors documented in the Cavium Octeon SDK. */
static void hardware_key(const unsigned char *key, size_t size)
{
	uint64_t words[4] = { 0 };
	uint64_t key_length = size / sizeof(words[0]) - 1;

	memcpy(words, key, size);
	__asm__ volatile(
		"dmtc2 %0,0x0104\n\t"
		"dmtc2 %1,0x0105\n\t"
		"dmtc2 %2,0x0106\n\t"
		"dmtc2 %3,0x0107\n\t"
		"dmtc2 %4,0x0110"
		:: "d"(words[0]), "d"(words[1]), "d"(words[2]), "d"(words[3]), "d"(key_length));
}

static void hardware_cbc(const unsigned char *iv, const unsigned char *in,
			 unsigned char *out, size_t size, bool encrypt)
{
	uint64_t first, second;

	memcpy(&first, iv, sizeof(first));
	memcpy(&second, iv + sizeof(first), sizeof(second));
	__asm__ volatile("dmtc2 %0,0x0102\n\tdmtc2 %1,0x0103" :: "d"(first), "d"(second));

	for (size_t offset = 0; offset < size; offset += BLOCK_SIZE) {
		uint64_t left, right;

		memcpy(&first, in + offset, sizeof(first));
		memcpy(&second, in + offset + sizeof(first), sizeof(second));
		if (encrypt) {
			__asm__ volatile(
				"dmtc2 %2,0x0108\n\tdmtc2 %3,0x3109\n\t"
				"dmfc2 %0,0x0100\n\tdmfc2 %1,0x0101"
				: "=&d"(left), "=&d"(right) : "d"(first), "d"(second));
		} else {
			__asm__ volatile(
				"dmtc2 %2,0x010c\n\tdmtc2 %3,0x310d\n\t"
				"dmfc2 %0,0x0100\n\tdmfc2 %1,0x0101"
				: "=&d"(left), "=&d"(right) : "d"(first), "d"(second));
		}
		memcpy(out + offset, &left, sizeof(left));
		memcpy(out + offset + sizeof(left), &right, sizeof(right));
	}
}
#else
static void check_hardware(void)
{
	fputs("Octeon COP2 requires a big-endian MIPS64 build. Use --software here.\n", stderr);
	exit(EXIT_FAILURE);
}

static void hardware_key(const unsigned char *key, size_t size)
{
	(void)key;
	(void)size;
	check_hardware();
}

static void hardware_cbc(const unsigned char *iv, const unsigned char *in,
			 unsigned char *out, size_t size, bool encrypt)
{
	(void)iv;
	(void)in;
	(void)out;
	(void)size;
	(void)encrypt;
	check_hardware();
}
#endif

static void decode_hex(unsigned char *out, size_t size, const char *hex)
{
	for (size_t i = 0; i < size; i++) {
		if (sscanf(hex + i * 2, "%2hhx", out + i) != 1) {
			fputs("Invalid AES test vector\n", stderr);
			exit(EXIT_FAILURE);
		}
	}
}

static double seconds(clockid_t id)
{
	struct timespec now;

	if (clock_gettime(id, &now) < 0) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}
	return now.tv_sec + now.tv_nsec / 1000000000.0;
}

static void verify(const struct aes_case *test, bool hardware, bool encrypt,
		   const unsigned char *result, const unsigned char *expected, size_t size)
{
	if (memcmp(result, expected, size)) {
		fprintf(stderr, "AES-%u-CBC %s %s: data mismatch at %zu bytes\n",
			test->bits, hardware ? "octeon-cop2" : "openssl",
			encrypt ? "encrypt" : "decrypt", size);
		exit(EXIT_FAILURE);
	}
}

static void check_vector(const struct aes_case *test, EVP_CIPHER_CTX **software,
			 const unsigned char *iv, bool hardware)
{
	unsigned char plain[2 * BLOCK_SIZE], cipher[2 * BLOCK_SIZE], output[3 * BLOCK_SIZE];

	decode_hex(plain, sizeof(plain), "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51");
	decode_hex(cipher, sizeof(cipher), test->expected);
	for (int encrypt = 0; encrypt <= 1; encrypt++) {
		const unsigned char *in = encrypt ? plain : cipher;
		const unsigned char *expected = encrypt ? cipher : plain;

		software_cbc(software[encrypt], iv, in, output, sizeof(plain));
		verify(test, false, encrypt, output, expected, sizeof(plain));
		if (hardware) {
			hardware_cbc(iv, in, output, sizeof(plain), encrypt);
			verify(test, true, encrypt, output, expected, sizeof(plain));
		}
	}
}

static void run(const struct aes_case *test, EVP_CIPHER_CTX *software, const unsigned char *iv,
		const unsigned char *in, const unsigned char *expected, unsigned char *out,
		size_t size, bool hardware, bool encrypt, bool check_only)
{
	unsigned int iterations = check_only ? 1 : (32 * MIB / size);
	double cpu_start = seconds(CLOCK_PROCESS_CPUTIME_ID);
	double start = seconds(CLOCK_MONOTONIC);

	if (hardware) {
		for (unsigned int i = 0; i < iterations; i++)
			hardware_cbc(iv, in, out, size, encrypt);
	} else {
		for (unsigned int i = 0; i < iterations; i++)
			software_cbc(software, iv, in, out, size);
	}
	double elapsed = seconds(CLOCK_MONOTONIC) - start;
	double cpu = seconds(CLOCK_PROCESS_CPUTIME_ID) - cpu_start;

	verify(test, hardware, encrypt, out, expected, size);
	printf("aes-%u-cbc %-12s %-7s %4zu KiB  data OK",
	       test->bits, hardware ? "octeon-cop2" : "openssl",
	       encrypt ? "encrypt" : "decrypt", size / KIB);
	if (!check_only)
		printf("  %7.2f MiB/s  %.3fs wall  %.3fs CPU",
		       (double)iterations * size / MIB / elapsed, elapsed, cpu);
	putchar('\n');
	fflush(stdout);
}

int main(int argc, char **argv)
{
	const size_t sizes[] = { 16 * KIB, MIB };
	unsigned char iv[BLOCK_SIZE];
	unsigned char *plain, *cipher, *output;
	bool check_only = false, hardware = true;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--check"))
			check_only = true;
		else if (!strcmp(argv[i], "--software"))
			hardware = false;
		else {
			fprintf(stderr, "Usage: %s [--check] [--software]\n", argv[0]);
			return EXIT_FAILURE;
		}
	}
	if (hardware)
		check_hardware();
	plain = malloc(MIB);
	cipher = malloc(MIB + BLOCK_SIZE);
	output = malloc(MIB + BLOCK_SIZE);
	if (!plain || !cipher || !output) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	decode_hex(iv, sizeof(iv), "000102030405060708090a0b0c0d0e0f");
	for (size_t i = 0; i < MIB; i++)
		plain[i] = (unsigned char)(i ^ (i >> 8));

	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		const struct aes_case *test = &cases[i];
		unsigned char key[32];
		size_t key_size = test->bits / 8;
		EVP_CIPHER_CTX *software[2];

		decode_hex(key, key_size, test->key);
		software[0] = software_context(test, key, false);
		software[1] = software_context(test, key, true);
		if (hardware)
			hardware_key(key, key_size);
		check_vector(test, software, iv, hardware);
		software_cbc(software[1], iv, plain, cipher, MIB);
		for (size_t j = 0; j < sizeof(sizes) / sizeof(sizes[0]); j++) {
			for (int encrypt = 1; encrypt >= 0; encrypt--) {
				const unsigned char *in = encrypt ? plain : cipher;
				const unsigned char *expected = encrypt ? cipher : plain;

				run(test, software[encrypt], iv, in, expected, output,
				    sizes[j], false, encrypt, check_only);
				if (hardware)
					run(test, software[encrypt], iv, in, expected, output,
					    sizes[j], true, encrypt, check_only);
			}
		}
		EVP_CIPHER_CTX_free(software[0]);
		EVP_CIPHER_CTX_free(software[1]);
	}
	free(output);
	free(cipher);
	free(plain);
	return EXIT_SUCCESS;
}
