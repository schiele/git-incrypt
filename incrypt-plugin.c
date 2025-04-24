/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8 -*- */

#include <stdio.h>
#include <string.h>
#include <openssl/err.h>
#include <openssl/evp.h>

int encryptdata(const unsigned char* input, size_t inputlen,
		unsigned char* output, size_t* outputlen,
		const unsigned char* key);
int decryptdata(const unsigned char* input, size_t inputlen,
		unsigned char* output, size_t* outputlen,
		const unsigned char* key);
void cmd_main(void);

static void handle_openssl_error(const char *message) {
    fprintf(stderr, "%s\n", message);
    ERR_print_errors_fp(stderr);
}

int encryptdata(const unsigned char* input, size_t inputlen,
		unsigned char* output, size_t* outputlen,
		const unsigned char* key) {
	const EVP_CIPHER *cipher_type = EVP_aes_256_cbc();
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	int outlen;
	if (ctx == NULL) {
		handle_openssl_error("EVP_CIPHER_CTX_new failed");
		return -1;
	}
	if (EVP_EncryptInit_ex(ctx, cipher_type, NULL, key, key+32) != 1) {
		handle_openssl_error("EVP_EncryptInit_ex failed");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}
	if (EVP_EncryptUpdate(ctx, output, &outlen, input, inputlen) != 1) {
		handle_openssl_error("EVP_EncryptUpdate failed");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}
	*outputlen = outlen;
	if (EVP_EncryptFinal_ex(ctx, output + *outputlen, &outlen) != 1) {
		handle_openssl_error("EVP_EncryptFinal_ex failed");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}
	*outputlen += outlen;
	EVP_CIPHER_CTX_free(ctx);
	return 0;
}

int decryptdata(const unsigned char* input, size_t inputlen,
		unsigned char* output, size_t* outputlen,
		const unsigned char* key) {
	const EVP_CIPHER *cipher_type = EVP_aes_256_cbc();
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	int outlen;
	if (ctx == NULL) {
		handle_openssl_error("EVP_CIPHER_CTX_new failed");
		return -1;
	}
	if (EVP_DecryptInit_ex(ctx, cipher_type, NULL, key, key+32) != 1) {
		handle_openssl_error("EVP_DecryptInit_ex failed");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}
	if (EVP_DecryptUpdate(ctx, output, &outlen, input, inputlen) != 1) {
		handle_openssl_error("EVP_DecryptUpdate failed");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}
	*outputlen = outlen;
	if (EVP_DecryptFinal_ex(ctx, output + *outputlen, &outlen) != 1) {
		handle_openssl_error("EVP_DecryptFinal_ex failed");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}
	*outputlen += outlen;
	EVP_CIPHER_CTX_free(ctx);
	return 0;
}

void cmd_main(void) {
}
