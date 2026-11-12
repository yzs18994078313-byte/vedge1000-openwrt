#define _POSIX_C_SOURCE 200809L
#define OPENSSL_SUPPRESS_DEPRECATED

#include <errno.h>
#include <linux/if_alg.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define KIB 1024
#define MIB (1024 * KIB)

struct algorithm {
	const char *name;
	const char *driver;
	size_t digest_size;
	const char *abc_digest;
	int (*software)(const void *, size_t, unsigned char *);
};

static int openssl_sha1(const void *data, size_t length, unsigned char *digest)
{
	SHA_CTX context;

	return SHA1_Init(&context) && SHA1_Update(&context, data, length)
		&& SHA1_Final(digest, &context);
}

static int openssl_sha256(const void *data, size_t length, unsigned char *digest)
{
	SHA256_CTX context;

	return SHA256_Init(&context) && SHA256_Update(&context, data, length)
		&& SHA256_Final(digest, &context);
}

static const struct algorithm algorithms[] = {
	{
		"sha1", "octeon-sha1", SHA_DIGEST_LENGTH,
		"a9993e364706816aba3e25717850c26c9cd0d89d", openssl_sha1
	},
	{
		"sha256", "octeon-sha256", SHA256_DIGEST_LENGTH,
		"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
		openssl_sha256
	}
};

static void fail(const char *operation)
{
	perror(operation);
	exit(EXIT_FAILURE);
}

static int hash_socket(const char *driver)
{
	struct sockaddr_alg address = { .salg_family = AF_ALG, .salg_type = "hash" };
	int control, operation;

	snprintf((char *)address.salg_name, sizeof(address.salg_name), "%s", driver);
	control = socket(AF_ALG, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
	if (control < 0)
		fail("AF_ALG socket");
	if (bind(control, (struct sockaddr *)&address, sizeof(address)) < 0)
		fail(driver);
	operation = accept(control, NULL, NULL);
	if (operation < 0)
		fail("AF_ALG accept");
	close(control);
	return operation;
}

static void send_hash_data(int fd, const void *data, size_t length, int flags)
{
	ssize_t count;

	do {
		count = send(fd, data, length, flags);
	} while (count < 0 && errno == EINTR);
	if (count < 0)
		fail("AF_ALG send");
	if ((size_t)count != length) {
		errno = EIO;
		fail("AF_ALG short send");
	}
}

static void read_hash(int fd, unsigned char *digest, size_t digest_size)
{
	ssize_t count;

	do {
		count = recv(fd, digest, digest_size, 0);
	} while (count < 0 && errno == EINTR);
	if (count < 0)
		fail("AF_ALG recv");
	if ((size_t)count != digest_size) {
		errno = EIO;
		fail("AF_ALG short digest");
	}
}

static void calculate(const struct algorithm *algorithm, int fd,
		      const void *data, size_t length, unsigned char *digest)
{
	if (fd >= 0) {
		send_hash_data(fd, data, length, 0);
		read_hash(fd, digest, algorithm->digest_size);
	} else if (!algorithm->software(data, length, digest)) {
		fprintf(stderr, "%s: OpenSSL digest failed\n", algorithm->name);
		exit(EXIT_FAILURE);
	}
}

static void verify_abc(const struct algorithm *algorithm, int fd, const char *backend)
{
	unsigned char digest[SHA256_DIGEST_LENGTH];
	char hex[2 * SHA256_DIGEST_LENGTH + 1];

	if (fd >= 0) {
		send_hash_data(fd, "a", 1, MSG_MORE);
		send_hash_data(fd, "bc", 2, 0);
		read_hash(fd, digest, algorithm->digest_size);
	} else {
		calculate(algorithm, fd, "abc", 3, digest);
	}
	for (size_t i = 0; i < algorithm->digest_size; i++)
		snprintf(hex + i * 2, 3, "%02x", (unsigned int)digest[i]);
	if (strcmp(hex, algorithm->abc_digest)) {
		fprintf(stderr, "%s %s: wrong digest for abc (%s)\n",
			algorithm->name, backend, hex);
		exit(EXIT_FAILURE);
	}
}

static double clock_seconds(clockid_t clock_id)
{
	struct timespec now;

	if (clock_gettime(clock_id, &now) < 0)
		fail("clock_gettime");
	return now.tv_sec + now.tv_nsec / 1000000000.0;
}

static void run(const struct algorithm *algorithm, int fd, const char *backend,
		const unsigned char *data, size_t length, int check_only)
{
	unsigned char expected[SHA256_DIGEST_LENGTH], actual[SHA256_DIGEST_LENGTH];
	unsigned int iterations = check_only ? 1 : (128 * MIB / length);
	double start, cpu_start, elapsed, cpu;

	calculate(algorithm, -1, data, length, expected);
	cpu_start = clock_seconds(CLOCK_PROCESS_CPUTIME_ID);
	start = clock_seconds(CLOCK_MONOTONIC);
	for (unsigned int i = 0; i < iterations; i++)
		calculate(algorithm, fd, data, length, actual);
	elapsed = clock_seconds(CLOCK_MONOTONIC) - start;
	cpu = clock_seconds(CLOCK_PROCESS_CPUTIME_ID) - cpu_start;
	if (memcmp(actual, expected, algorithm->digest_size)) {
		fprintf(stderr, "%s %s: digest mismatch at %zu bytes\n",
			algorithm->name, backend, length);
		exit(EXIT_FAILURE);
	}
	printf("%-7s %-14s %4zu KiB  digest OK", algorithm->name, backend, length / KIB);
	if (!check_only)
		printf("  %7.2f MiB/s  %.3fs wall  %.3fs CPU",
		       (double)iterations * length / MIB / elapsed, elapsed, cpu);
	putchar('\n');
	fflush(stdout);
}

int main(int argc, char **argv)
{
	const size_t lengths[] = {16 * KIB, MIB};
	int check_only = 0, kernel_default = 0, software_only = 0;
	unsigned char *data;
	uint32_t random = 0x6a09e667;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--check"))
			check_only = 1;
		else if (!strcmp(argv[i], "--kernel"))
			kernel_default = 1;
		else if (!strcmp(argv[i], "--software"))
			software_only = 1;
		else {
			fprintf(stderr, "Usage: %s [--check] [--kernel | --software]\n", argv[0]);
			return EXIT_FAILURE;
		}
	}
	if (kernel_default && software_only) {
		fprintf(stderr, "--kernel and --software are mutually exclusive\n");
		return EXIT_FAILURE;
	}
	data = malloc(MIB);
	if (!data)
		fail("malloc");
	for (size_t i = 0; i < MIB; i++) {
		random ^= random << 13;
		random ^= random >> 17;
		random ^= random << 5;
		data[i] = (unsigned char)random;
	}

	for (size_t i = 0; i < sizeof(algorithms) / sizeof(algorithms[0]); i++) {
		const struct algorithm *algorithm = &algorithms[i];
		const char *driver = kernel_default ? algorithm->name : algorithm->driver;
		int fd = software_only ? -1 : hash_socket(driver);

		verify_abc(algorithm, -1, "openssl");
		if (fd >= 0)
			verify_abc(algorithm, fd, driver);
		for (size_t j = 0; j < sizeof(lengths) / sizeof(lengths[0]); j++) {
			run(algorithm, -1, "openssl", data, lengths[j], check_only);
			if (fd >= 0)
				run(algorithm, fd, driver, data, lengths[j], check_only);
		}
		if (fd >= 0)
			close(fd);
	}
	free(data);
	return EXIT_SUCCESS;
}
