ifeq ($(OS),Windows_NT)
  PATHSEP = ;
else
  PATHSEP = :
endif
export GIT_EXEC_PATH := $(CURDIR)$(PATHSEP)$(shell git --exec-path)
export MANPATH := $(CURDIR)$(PATHSEP)$(MANPATH)
VERBOSE :=
TESTREPO := $(CURDIR)
KEY := 5A8A11E44AD2A1623B84E5AFC5C0C5C7218D18D7
REPO := incrypt::$(CURDIR)/crypt

.PHONY: all man test clean

all: man

ifndef NODOC
man: man1/git-incrypt.1
testman: man
	PAGER= git incrypt --help
else
man:
testman:
endif

man1/%.1: man1/%.xml manpage-normal.xsl manpage-bold-literal.xsl
	cd man1 && xmlto $(patsubst %,-m ../%,$(wordlist 2, 3, $^)) man ../$<

man1/%.xml: %.adoc asciidoc.conf
	mkdir -p man1
	asciidoc -f $(word 2, $^) -b docbook -d manpage -o $@ $<

test: testman
	rm -rf crypt tst
	mkdir crypt
	git -C crypt init --bare -b _
	git ls-remote $(TESTREPO)
	git incrypt init $(REPO) $(KEY)
	git -C $(TESTREPO) fetch $(REPO) || git -C $(TESTREPO) incrypt trust $(REPO)
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

clean:
	rm -rf man1 crypt tst
