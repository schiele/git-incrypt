/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8 -*- */

#include <stdio.h>
#include <string.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#define USE_THE_REPOSITORY_VARIABLE

#include "git-compat-util.h"
#include "hash.h"
#include "hex.h"
#include "setup.h"
#include "ident.h"
#include "run-command.h"
#include "odb.h"
//#include "read-cache-ll.h"
//#include "cache-tree.h"
#include "strbuf.h"
#include "sigchain.h"
#include "gettext.h"
//#include "config.h"
//#include "environment.h"

void globalinit(const char* dir, const char* url_arg);
int set_option(const char *name, size_t namelen, const char *value);
int getverbosity(void);
int getprogress(void);
int getatomic(void);
const char* geturl(void);
const char* getprefix(void);
void setcryptkey(const unsigned char* k);
const unsigned char* getcryptkey(void);
unsigned char* encryptdata(const unsigned char* input, size_t inputlen,
			   unsigned char* output, size_t* outputlen);
unsigned char* decryptdata(const unsigned char* input, size_t inputlen,
			   unsigned char* output, size_t* outputlen);
char* encryptrefname(const char* input, char* output);
char* decryptrefname(const char* input, char* output);
unsigned char* hashdata(const unsigned char* input, size_t inputlen,
			unsigned char* output);
char* hashdatahex(const unsigned char* input, size_t inputlen,
		  char* output);
void hashdatabuf(struct strbuf* out, struct strbuf* in);
void hashdatabufhex(struct strbuf* out, struct strbuf* in);
void initbare(const char* dir);
char* mktemplate(const char* name, const char* email, const char* date, const char* msg);
void fetchpattern(const char pattern);
void metainit(void);
char* writemeta(char* output);
int encrypt_buffer_gpg(struct strbuf *buffer, struct strbuf *output,
		       struct string_list *recipients);
void myupdaterefs(const char* refname, const char* oid);

/*static*/ const char* CRYPTREADME = "# 401 Unauthorized\n\n"
"This is an encrypted git repository.  You can clone it, but you will not be\n"
"able to see the contents of the commits.  If you have the right key, you can\n"
"decrypt the repository using\n"
"[git-incrypt](https://github.com/schiele/git-incrypt).\n";

struct options {
	int verbosity;
	unsigned progress : 1,
		atomic : 1;
};
static struct options options;

static struct strbuf url = STRBUF_INIT;
static struct strbuf prefix = STRBUF_INIT;

void globalinit(const char* dir, const char* url_arg) {
	chdir(dir);
	options.verbosity = 1;
	options.progress = !!isatty(2);
	options.atomic = 0;
	strbuf_addstr(&url, url_arg);
	strbuf_addstr(&prefix, "refs/incrypt/");
	hashdatabufhex(&prefix, &url);
	strbuf_addch(&prefix, '/');
}

/*static*/ int set_option(const char *name, size_t namelen, const char *value)
{
	if (!strncmp(name, "verbosity", namelen)) {
		char *end;
		int v = strtol(value, &end, 10);
		if (value == end || *end)
			return -1;
		options.verbosity = v;
		return 0;
	}
	else if (!strncmp(name, "progress", namelen)) {
		if (!strcmp(value, "true"))
			options.progress = 1;
		else if (!strcmp(value, "false"))
			options.progress = 0;
		else
			return -1;
		return 0;
	}
	else if (!strncmp(name, "followtags", namelen)) {
		return 0;
	} else if (!strncmp(name, "atomic", namelen)) {
		if (!strcmp(value, "true"))
			options.atomic = 1;
		else if (!strcmp(value, "false"))
			options.atomic = 0;
		else
			return -1;
		return 0;
	} else {
		return 1 /* unsupported */;
	}
}

int getverbosity(void)
{
	return options.verbosity;
}

int getprogress(void)
{
	return options.progress;
}

int getatomic(void)
{
	return options.atomic;
}

const char* geturl(void) {
	return url.buf;
}

const char* getprefix(void) {
	return prefix.buf;
}

static unsigned char key[48];

void setcryptkey(const unsigned char* k) {
	memcpy(key, k, 48);
}

const unsigned char* getcryptkey(void) {
	return key;
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

unsigned char* hashdata(const unsigned char* input, size_t inputlen,
			unsigned char* output) {
	struct git_hash_ctx c;
	hash_algos[GIT_HASH_SHA1].init_fn(&c);
	git_hash_update(&c, input, inputlen);
	git_hash_final(output, &c);
	return output;
}

char* hashdatahex(const unsigned char* input, size_t inputlen, char* output) {
	unsigned char hash[GIT_SHA1_RAWSZ];
	hashdata(input, inputlen, hash);
	return hash_to_hex_algop_r(output, hash, &hash_algos[GIT_HASH_SHA1]);
}

void hashdatabuf(struct strbuf* out, struct strbuf* in) {
	unsigned char hash[GIT_SHA1_RAWSZ];
	hashdata((const unsigned char*)in->buf, in->len, hash);
	strbuf_add(out, hash, GIT_SHA1_RAWSZ);
}

void hashdatabufhex(struct strbuf* out, struct strbuf* in) {
	unsigned char hash[GIT_SHA1_RAWSZ];
	hashdata((const unsigned char*)in->buf, in->len, hash);
	strbuf_addstr(out, hash_to_hex_algop(hash, &hash_algos[GIT_HASH_SHA1]));
}

void initbare(const char* dir) {
	init_db(dir, NULL, NULL, GIT_HASH_UNKNOWN, REF_STORAGE_FORMAT_UNKNOWN, NULL, -1, 0);
}

struct strbuf template = STRBUF_INIT;

char* mktemplate(const char* name, const char* email, const char* date, const char* msg) {
        if (!msg)
		msg = "Encrypted by git-incrypt.\n\n"
                      "https://github.com/schiele/git-incrypt\n";
	strbuf_addf(&template, "author %s\n", fmt_ident(name?name:getenv("GIT_AUTHOR_NAME"), email?email:getenv("GIT_AUTHOR_EMAIL"), WANT_AUTHOR_IDENT, date?date:getenv("GIT_AUTHOR_DATE"), 0));
	strbuf_addf(&template, "committer %s\n\n", fmt_ident(name?name:getenv("GIT_COMMITTER_NAME"), email?email:getenv("GIT_COMMITTER_EMAIL"), WANT_COMMITTER_IDENT, date?date:getenv("GIT_COMMITTER_DATE"), 0));
	strbuf_add(&template, msg, strlen(msg));
	return template.buf;
}

static const char* verbosityflags[5] = {"-q", "-q", "-v", "-vv", "-vvv"};
static const char* progressflags[2] = {"--no-progress", "--progress"};

void fetchpattern(const char pattern) {
	struct child_process cmd = CHILD_PROCESS_INIT;

	struct strbuf refspec = STRBUF_INIT;
	strbuf_addf(&refspec, "+refs/heads/%c:%s1/%c", pattern, prefix.buf, pattern);

	strvec_pushl(&cmd.args, "fetch", verbosityflags[options.verbosity],
		     progressflags[options.progress], "--no-write-fetch-head",
		     "-p", url.buf, refspec.buf, NULL);
	strbuf_release(&refspec);

	cmd.git_cmd = 1;
	//This causes a crash! : cmd.close_object_store = 1;
	/*return*/ run_command(&cmd);
}

void myupdaterefs(const char* refname, const char* oid) {
	struct child_process cmd = CHILD_PROCESS_INIT;
	strvec_pushl(&cmd.args, "update-ref", refname, oid, NULL);
	cmd.git_cmd = 1;
	//This causes a crash! : cmd.close_object_store = 1;
	/*return*/ run_command(&cmd);
}

const char* ver = "git-incrypt\n1.0.0\n";
const char* keyver = "AES-256-CBC+IV";
struct object_id obj_ver;
struct object_id obj_key;
struct object_id obj_sig;
struct object_id obj_msg;
struct object_id obj_def;

void metainit(void) {
	struct strbuf keybuf = STRBUF_INIT;
	struct strbuf output_buf = STRBUF_INIT;
	//char* key[48];
	struct string_list recipients = STRING_LIST_INIT_NODUP;
	struct strbuf templateprefixed = STRBUF_INIT;
	unsigned char* templateencrypted = NULL;
	size_t templateencryptedlen = 0;
	struct strbuf defaultbranch = STRBUF_INIT;
	struct strbuf defaultbranchprefixed = STRBUF_INIT;
	unsigned char* defaultbranchencrypted = NULL;
	size_t defaultbranchencryptedlen = 0;
        odb_write_object(the_repository->objects, ver, strlen(ver), OBJ_BLOB, &obj_ver);
	strbuf_add(&keybuf, keyver, 15);
	getrandom(key, 48, 0);
	strbuf_add(&keybuf, key, 48);
	/* This hard coded value needs to be replaced later! */
	string_list_append(&recipients, "5A8A11E44AD2A1623B84E5AFC5C0C5C7218D18D7");
	if (encrypt_buffer_gpg(&keybuf, &output_buf, &recipients) < 0)
    		die("Encryption failed");
	strbuf_release(&keybuf);
	odb_write_object(the_repository->objects, output_buf.buf, output_buf.len, OBJ_BLOB, &obj_key);
	strbuf_release(&output_buf);
	odb_write_object(the_repository->objects, NULL, 0, OBJ_TREE, &obj_sig);
	hashdatabuf(&templateprefixed, &template);
	strbuf_add(&templateprefixed, template.buf, template.len);
	templateencrypted = malloc(templateprefixed.len+16);
	encryptdata((const unsigned char*)templateprefixed.buf, templateprefixed.len, templateencrypted, &templateencryptedlen);
	odb_write_object(the_repository->objects, templateencrypted, templateencryptedlen, OBJ_BLOB, &obj_msg);
	strbuf_release(&templateprefixed);
	free(templateencrypted);
	strbuf_addf(&defaultbranch, "refs/heads/%s", "master");
	hashdatabuf(&defaultbranchprefixed, &defaultbranch);
	strbuf_add(&defaultbranchprefixed, defaultbranch.buf, defaultbranch.len);
	defaultbranchencrypted = malloc(defaultbranchprefixed.len+16);
	encryptdata((const unsigned char*)defaultbranchprefixed.buf, defaultbranchprefixed.len, defaultbranchencrypted, &defaultbranchencryptedlen);
	odb_write_object(the_repository->objects, defaultbranchencrypted, defaultbranchencryptedlen, OBJ_BLOB, &obj_def);
	strbuf_release(&defaultbranchprefixed);
	free(defaultbranchencrypted);
}

static void secretcommit(struct object_id* tid, struct object_id* oid) {
	struct strbuf commit = STRBUF_INIT;
	strbuf_addf(&commit, "tree %s\n", oid_to_hex(tid));
	strbuf_add(&commit, template.buf, template.len);
	odb_write_object(the_repository->objects, commit.buf, commit.len, OBJ_COMMIT, oid);
	strbuf_release(&commit);
}

char* writemeta(char* output) {
	struct object_id oid;
	struct object_id tid;
	struct strbuf tb = STRBUF_INIT;
	struct strbuf map = STRBUF_INIT;
	struct strbuf mapprefixed = STRBUF_INIT;
	unsigned char* mapencrypted = NULL;
	size_t mapencryptedlen = 0;
	struct object_id obj_readme;
	struct object_id obj_map;
	struct strbuf refname = STRBUF_INIT;
        odb_write_object(the_repository->objects, CRYPTREADME, strlen(CRYPTREADME), OBJ_BLOB, &obj_readme);
	strbuf_addf(&tb, "%o %s%c", 0100644, "README.md", '\0');
	strbuf_add(&tb, obj_readme.hash, the_hash_algo->rawsz);
	strbuf_addf(&tb, "%o %s%c", 0100644, "def", '\0');
	strbuf_add(&tb, obj_def.hash, the_hash_algo->rawsz);
	strbuf_addf(&tb, "%o %s%c", 0100644, "key", '\0');
	strbuf_add(&tb, obj_key.hash, the_hash_algo->rawsz);
	hashdatabuf(&mapprefixed, &map);
	strbuf_add(&mapprefixed, map.buf, map.len);
	mapencrypted = malloc(mapprefixed.len+16);
	encryptdata((const unsigned char*)mapprefixed.buf, mapprefixed.len, mapencrypted, &mapencryptedlen);
	odb_write_object(the_repository->objects, mapencrypted, mapencryptedlen, OBJ_BLOB, &obj_map);
	strbuf_release(&mapprefixed);
	free(mapencrypted);
	strbuf_addf(&tb, "%o %s%c", 0100644, "map", '\0');
	strbuf_add(&tb, obj_map.hash, the_hash_algo->rawsz);
	strbuf_addf(&tb, "%o %s%c", 0100644, "msg", '\0');
	strbuf_add(&tb, obj_msg.hash, the_hash_algo->rawsz);
	strbuf_addf(&tb, "%o %s%c", 0100755, "sig", '\0');
	strbuf_add(&tb, obj_sig.hash, the_hash_algo->rawsz);
	strbuf_addf(&tb, "%o %s%c", 0100644, "ver", '\0');
	strbuf_add(&tb, obj_ver.hash, the_hash_algo->rawsz);
	odb_write_object(the_repository->objects, tb.buf, tb.len, OBJ_TREE, &tid);
	strbuf_release(&tb);
	secretcommit(&tid, &oid);
	strbuf_addbuf(&refname, &prefix);
	strbuf_addstr(&refname, "1/_");
	// refs_update_ref(get_main_ref_store(the_repository), NULL, refname.buf,
	//		&oid, NULL, 0, UPDATE_REFS_MSG_ON_ERR);
	myupdaterefs(refname.buf, oid_to_hex(&oid));
	strbuf_release(&refname);
	oid_to_hex_r(output, &oid);
	return output;
}

int encrypt_buffer_gpg(struct strbuf *buffer, struct strbuf *output,
		       struct string_list *recipients)
{
	struct child_process gpg = CHILD_PROCESS_INIT;
	int ret;
	const char *cp;
	struct strbuf gpg_status = STRBUF_INIT;
	struct string_list_item *item;

	strvec_pushl(&gpg.args, "gpg", "-e", "--status-fd=2", NULL);

	for_each_string_list_item(item, recipients) {
		strvec_pushl(&gpg.args, "-r", item->string, NULL);
	}

	sigchain_push(SIGPIPE, SIG_IGN);
	ret = pipe_command(&gpg, buffer->buf, buffer->len,
			   output, 1024, &gpg_status, 0);
	sigchain_pop(SIGPIPE);

	for (cp = gpg_status.buf;
	     cp && (cp = strstr(cp, "[GNUPG:] BEGIN_ENCRYPTION "));
	     cp++) {
		if (cp == gpg_status.buf || cp[-1] == '\n')
			break;
	}

	ret |= !cp;
	if (ret) {
		error(_("gpg failed to encrypt the data:\n%s"),
		      gpg_status.len ? gpg_status.buf : "(no gpg output)");
		strbuf_release(&gpg_status);
		return -1;
	}

	strbuf_release(&gpg_status);
	return 0;
}

int cmd_main(int argc, const char** argv) {
	(void)argv;
	(void)argc;
	(void)base64decode;
	return 0;
}
