/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8 -*- */

#define USE_THE_REPOSITORY_VARIABLE

#include "git-compat-util.h"
#include "gettext.h"
#include "hex.h"
#include "remote.h"
#include "strbuf.h"
#include "setup.h"
#include "trace2.h"

static struct remote *remote;
static const char* url;

struct options {
	int verbosity;
	unsigned progress : 1,
		atomic : 1;
};
static struct options options;

static int set_option(const char *name, size_t namelen, const char *value)
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

static struct ref *get_refs(void)
{
	return NULL;
}

static void output_refs(struct ref *refs)
{
	struct ref *posn;
	for (posn = refs; posn; posn = posn->next) {
		if (posn->symref)
			printf("@%s %s\n", posn->symref, posn->name);
		else
			printf("%s %s\n", hash_to_hex_algop(posn->old_oid.hash,
							    &hash_algos[GIT_HASH_SHA1]),
					  posn->name);
	}
	printf("\n");
	fflush(stdout);
}

static int fetch(int nr_heads, struct ref **to_fetch)
{
	(void)nr_heads;
	(void)to_fetch;
	return 0;
}

static void parse_fetch(struct strbuf *buf)
{
	struct ref **to_fetch = NULL;
	struct ref *list_head = NULL;
	struct ref **list = &list_head;
	int alloc_heads = 0, nr_heads = 0;

	do {
		const char *p;
		if (skip_prefix(buf->buf, "fetch ", &p)) {
			const char *name;
			struct ref *ref;
			struct object_id old_oid;
			const char *q;

			if (parse_oid_hex(p, &old_oid, &q))
				die(_("protocol error: expected sha/ref, got '%s'"), p);
			if (*q == ' ')
				name = q + 1;
			else if (!*q)
				name = "";
			else
				die(_("protocol error: expected sha/ref, got '%s'"), p);

			ref = alloc_ref(name);
			oidcpy(&ref->old_oid, &old_oid);

			*list = ref;
			list = &ref->next;

			ALLOC_GROW(to_fetch, nr_heads + 1, alloc_heads);
			to_fetch[nr_heads++] = ref;
		}
		else
			die(_("remote-inctypt does not support %s"), buf->buf);

		strbuf_reset(buf);
		if (strbuf_getline_lf(buf, stdin) == EOF)
			return;
		if (!*buf->buf)
			break;
	} while (1);

	if (fetch(nr_heads, to_fetch))
		exit(128); /* error already reported */
	free_refs(list_head);
	free(to_fetch);

	printf("\n");
	fflush(stdout);
	strbuf_reset(buf);
}

static int tool_main(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	error(_("tool not implemented"));
	return 1;
}

static int remote_main(int argc, const char **argv)
{
	struct strbuf buf = STRBUF_INIT;
	int nongit;
	int ret = 1;

	setup_git_directory_gently(&nongit);
	if (argc < 2) {
		error(_("remote-incrypt: usage: git remote-incrypt <remote> [<url>]"));
		goto cleanup;
	}

	options.verbosity = 1;
	options.progress = !!isatty(2);

	trace2_cmd_name("incrypt");

	remote = remote_get(argv[1]);

	if (argc > 2) {
		url = argv[2];
	} else {
		url = remote->url.v[0];
	}

	do {
		const char *arg;

		if (strbuf_getline_lf(&buf, stdin) == EOF) {
			if (ferror(stdin))
				error(_("remote-incrypt: error reading command stream from git"));
			goto cleanup;
		}
		if (buf.len == 0)
			break;
		if (starts_with(buf.buf, "fetch ")) {
			if (nongit) {
				setup_git_directory_gently(&nongit);
				if (nongit)
					die(_("remote-incrypt: fetch attempted without a local repo"));
			}
			parse_fetch(&buf);

		} else if (!strcmp(buf.buf, "list") || starts_with(buf.buf, "list ")) {
			output_refs(get_refs());

		} else if (skip_prefix(buf.buf, "option ", &arg)) {
			const char *value = strchrnul(arg, ' ');
			size_t arglen = value - arg;
			int result;

			if (*value)
				value++; /* skip over SP */
			else
				value = "true";

			result = set_option(arg, arglen, value);
			if (!result)
				printf("ok\n");
			else if (result < 0)
				printf("error invalid value\n");
			else
				printf("unsupported\n");
			fflush(stdout);

		} else if (!strcmp(buf.buf, "capabilities")) {
			printf("fetch\n");
			printf("option\n");
			printf("push\n");
			printf("\n");
			fflush(stdout);
		} else {
			error(_("remote-incrypt: unknown command '%s' from git"), buf.buf);
			goto cleanup;
		}
		strbuf_reset(&buf);
	} while (1);

	ret = 0;
cleanup:
	strbuf_release(&buf);

	return ret;
}

int cmd_main(int argc, const char **argv)
{
	const char* base = strrchr(argv[0], '/');
	if (base)
		++base;
	else
		base = argv[0];
	if (!strcmp(base, "git-remote-incrypt"))
		return remote_main(argc, argv);
	else if (!strcmp(base, "git-incrypt"))
		return tool_main(argc, argv);
	else {
		error(_("command not implemented: %s"), base);
		return 1;
	}
}
