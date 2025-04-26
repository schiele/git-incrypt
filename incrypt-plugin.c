/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8 -*- */

#include <stdio.h>
#include <string.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#include "git-compat-util.h"
#include "hash.h"

void setcryptkey(const unsigned char* k);
unsigned char* encryptdata(const unsigned char* input, size_t inputlen,
			   unsigned char* output, size_t* outputlen);
unsigned char* decryptdata(const unsigned char* input, size_t inputlen,
			   unsigned char* output, size_t* outputlen);
char* encryptrefname(const char* input, char* output);
char* decryptrefname(const char* input, char* output);

static unsigned char key[48];

void setcryptkey(const unsigned char* k) {
	memcpy(key, k, 48);
}

static void handle_openssl_error(const char *message) {
    fprintf(stderr, "%s\n", message);
    ERR_print_errors_fp(stderr);
}

struct cryptobj {
	EVP_CIPHER_CTX *ctx;
	unsigned char *output;
	size_t outlen;
};

static struct cryptobj* encrypt_init(unsigned char* output) {
	struct cryptobj* co = malloc(sizeof(struct cryptobj));
	if (co == NULL)
		return NULL;
	co->ctx = EVP_CIPHER_CTX_new();
	if (co->ctx == NULL) {
		handle_openssl_error("EVP_CIPHER_CTX_new failed");
		free(co);
		return NULL;
	}
	if (EVP_EncryptInit_ex(co->ctx, EVP_aes_256_cbc(), NULL, key, key+32) != 1) {
		handle_openssl_error("EVP_EncryptInit_ex failed");
		EVP_CIPHER_CTX_free(co->ctx);
		free(co);
		return NULL;
	}
	co->output = output;
	co->outlen = 0;
	return co;
}

static struct cryptobj* encrypt_update(struct cryptobj* co,
				       const unsigned char* input, size_t inputlen) {
	int outlen;
	if (EVP_EncryptUpdate(co->ctx, co->output + co->outlen, &outlen, input, inputlen) != 1) {
		handle_openssl_error("EVP_EncryptUpdate failed");
		EVP_CIPHER_CTX_free(co->ctx);
		free(co);
		return NULL;
	}
	co->outlen += outlen;
	return co;
}

static unsigned char* encrypt_final(struct cryptobj* co, size_t* outputlen) {
	int outlen;
	unsigned char* out = co->output;
	if (EVP_EncryptFinal_ex(co->ctx, co->output + co->outlen, &outlen) != 1) {
		handle_openssl_error("EVP_EncryptFinal_ex failed");
		EVP_CIPHER_CTX_free(co->ctx);
		free(co);
		return NULL;
	}
	co->outlen += outlen;
	*outputlen = co->outlen;
	EVP_CIPHER_CTX_free(co->ctx);
	free(co);
	return out;
}

unsigned char* encryptdata(const unsigned char* input, size_t inputlen,
		unsigned char* output, size_t* outputlen) {
	struct cryptobj* co = encrypt_init(output);
	encrypt_update(co, input, inputlen);
	return encrypt_final(co, outputlen);
}

unsigned char* decryptdata(const unsigned char* input, size_t inputlen,
			   unsigned char* output, size_t* outputlen) {
	const EVP_CIPHER *cipher_type = EVP_aes_256_cbc();
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	int outlen;
	if (ctx == NULL) {
		handle_openssl_error("EVP_CIPHER_CTX_new failed");
		return NULL;
	}
	if (EVP_DecryptInit_ex(ctx, cipher_type, NULL, key, key+32) != 1) {
		handle_openssl_error("EVP_DecryptInit_ex failed");
		EVP_CIPHER_CTX_free(ctx);
		return NULL;
	}
	if (EVP_DecryptUpdate(ctx, output, &outlen, input, inputlen) != 1) {
		handle_openssl_error("EVP_DecryptUpdate failed");
		EVP_CIPHER_CTX_free(ctx);
		return NULL;
	}
	*outputlen = outlen;
	if (EVP_DecryptFinal_ex(ctx, output + *outputlen, &outlen) != 1) {
		handle_openssl_error("EVP_DecryptFinal_ex failed");
		EVP_CIPHER_CTX_free(ctx);
		return NULL;
	}
	*outputlen += outlen;
	EVP_CIPHER_CTX_free(ctx);
	return output;
}

static void base64encode(const unsigned char* input, size_t inputlen,
		 unsigned char* output, size_t* outputlen) {
	static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+#=";
	size_t r = 0, w = 0;
	while (r < inputlen) {
		unsigned char b[5] = { 0, 0, 0, 0, 0 };
		int l = r + 3 < inputlen ? 3 : (inputlen - r);
		memcpy(b + 1, input + r, l);
		for (int i = 0; i < 4; ++i)
			output[w + i] =
				b64[(l > i - 1) ?
				    ((b[i + 0] << ((3 - i) << 1)) |
				     (b[i + 1] >> ((1 + i) << 1))) & 0x3f :
				    64];
		r += 3;
		w += 4;
	}
	*outputlen = w;
}

static void base64decode(const unsigned char* input, size_t inputlen,
		 unsigned char* output, size_t* outputlen) {
	static const unsigned char b64[] = {
		-1, -1, -1, 63, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, -1,
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, 64, -1, -1,
		-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
		-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
	};
	unsigned char b[4] = { 0, 0, 0, 0 };
	size_t r = 0, w = 0;
	while (r < inputlen) {
		for (int i = 0; i < 4; ++i)
			b[i] = b64[input[r + i] - 32];
		for (int i = 0; i < 3; ++i)
			output[w + i] =
				(b[i + 0] << ((1 + i) << 1)) |
				(b[i + 1] >> ((2 - i) << 1));
		r += 4;
		w += 3;
	}
	*outputlen = w - (b[2] >= 64 ? 2 : (b[3] >= 64 ? 1 : 0));
}

char* encryptrefname(const char* input, char* output) {
	struct git_hash_ctx c;
	size_t inputlen = strlen(input);
	size_t hashlen = hash_algos[GIT_HASH_SHA1].rawsz;
	unsigned char* buf1 = malloc(hashlen);
	unsigned char* buf2 = malloc(hashlen + inputlen + AES_BLOCK_SIZE);
	size_t buf2len;
	size_t outlen;
	struct cryptobj* co = encrypt_init(buf2);
	hash_algos[GIT_HASH_SHA1].init_fn(&c);
	git_hash_update(&c, input, inputlen);
	git_hash_final(buf1, &c);
	encrypt_update(co, buf1, hashlen);
	encrypt_update(co, (const unsigned char*)input, inputlen);
	encrypt_final(co, &buf2len);
	base64encode(buf2, buf2len, (unsigned char*)output, &outlen);
	output[outlen] = '\0';
	free(buf1);
	free(buf2);
	return output;
}

char* decryptrefname(const char* input, char* output) {
	const char* data = strrchr(input, '/');
	size_t inputlen;
	unsigned char* buf1;
	unsigned char* buf2;
	size_t outlen1;
	size_t outlen2;
	size_t hashlen = hash_algos[GIT_HASH_SHA1].rawsz;
	data = data ? data + 1 : input;
	inputlen = strlen(data);
        buf1 = malloc(inputlen * 3 / 4);
	buf2 = malloc(inputlen * 3 / 4);
	base64decode((const unsigned char*)data, inputlen, buf1, &outlen1);
	decryptdata((const unsigned char*)buf1, outlen1, buf2, &outlen2);
	memcpy(output, buf2 + hashlen, outlen2 - hashlen);
	output[outlen2 - hashlen] = '\0';
	return output;
}

int cmd_main(int argc, const char** argv) {
	(void)argv;
	(void)argc;
	(void)base64decode;
	return 0;
}
