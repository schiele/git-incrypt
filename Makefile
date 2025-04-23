ifeq ($(OS),Windows_NT)
  PATHSEP = ;
else
  PATHSEP = :
endif
ORIG_EXEC_PATH := $(shell git --exec-path)
export GIT_EXEC_PATH := $(CURDIR)$(PATHSEP)$(ORIG_EXEC_PATH)
export MANPATH := $(CURDIR)$(PATHSEP)$(MANPATH)
VERBOSE :=
TESTREPO := $(CURDIR)
KEY := 5A8A11E44AD2A1623B84E5AFC5C0C5C7218D18D7
REPO := incrypt::$(CURDIR)/crypt

.PHONY: all man test clean

all: git-remote-incrypt man

ifndef NODOC
man: man1/git-incrypt.1
testman: man
	PAGER= git incrypt --help
else
man:
testman:
endif

git-%: git/git-%
	cp $< $@

git/git-remote-incrypt: git/incrypt.c git/config.mak
	$(MAKE) -C $(@D) DEVELOPER:=1 $(@F)

git/%.c: %.c
	cp $< $@

man1/%.1: git/Documentation/%.1
	mkdir -p man1
	cp $< $@

git/Documentation/%.1: git/Documentation/%.adoc git/config.mak
	$(MAKE) -C $(@D) DEVELOPER:=1 $(@F)

git/Documentation/%.adoc: %.adoc
	cp $< $@

git/config.mak: config.mak
	cp $< $@

test: testman
	rm -rf crypt tst
	mkdir crypt
	git -C crypt init --bare -b _
	git ls-remote $(TESTREPO)
	git incrypt init $(REPO) $(KEY)
	git -C $(TESTREPO) fetch $(VERBOSE) $(REPO) || git -C $(TESTREPO) incrypt trust $(REPO)
	git -C $(TESTREPO) push $(VERBOSE) $(REPO) HEAD~2:refs/heads/master
	git clone $(VERBOSE) $(REPO) tst
	git -C $(TESTREPO) push $(VERBOSE) $(REPO) v0.9.0
	git -C tst pull $(VERBOSE)
	git -C $(TESTREPO) push $(VERBOSE) $(REPO) HEAD:master
	git -C tst pull $(VERBOSE)
	git -C $(TESTREPO) push $(VERBOSE) -f --all $(REPO)
	git -C tst pull --no-rebase -X theirs --no-edit $(VERBOSE)
	git ls-remote crypt
	git -C tst ls-remote $(REPO)
	git ls-remote tst
	pylint git-incrypt
	pycodestyle git-incrypt

PKGNAME := git-incrypt
LICENSEDIR := /usr/share/licenses
DOCDIR := /usr/share/doc
MANDIR := /usr/share/man

install: man
	install -D -m 755 -t $(DESTDIR)$(ORIG_EXEC_PATH) git-incrypt
	ln -s git-incrypt $(DESTDIR)$(ORIG_EXEC_PATH)/git-remote-incrypt
	install -D -m 644 -t $(DESTDIR)$(LICENSEDIR)/$(PKGNAME) COPYING
	install -D -m 644 -t $(DESTDIR)$(DOCDIR)/$(PKGNAME) FORMAT.md README.md
	install -D -m 644 -t $(DESTDIR)$(MANDIR)/man1 man1/git-incrypt.1

clean:
	$(MAKE) -C git clean
	rm -rf git-remote-incrypt man1 crypt tst \
		git/config.mak git/*incrypt* git/Documentation/*incrypt*
