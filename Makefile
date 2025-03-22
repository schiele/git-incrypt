export PATH := $(CURDIR):$(PATH)
export MANPATH := $(CURDIR):$(MANPATH)
VERBOSE :=
TESTREPO := ~/sysconf
KEY := 5A8A11E44AD2A1623B84E5AFC5C0C5C7218D18D7
REPO := incrypt::$(CURDIR)/crypt

.PHONY: all man test clean

all: man
man: man1/git-incrypt.1

man1/%.1: man1/%.xml manpage-normal.xsl manpage-bold-literal.xsl
	cd man1 && xmlto $(patsubst %,-m ../%,$(wordlist 2, 3, $^)) man ../$<

man1/%.xml: %.adoc asciidoc.conf
	mkdir -p man1
	asciidoc -f $(word 2, $^) -b docbook -d manpage -o $@ $<

test: man
	PAGER= git incrypt --help
	rm -rf crypt tst
	mkdir crypt
	git -C crypt init --bare -b _
	git ls-remote $(TESTREPO)
	git incrypt init $(REPO) $(KEY)
	git -C $(TESTREPO) fetch $(REPO) || git -C $(TESTREPO) incrypt trust $(REPO)
	git -C $(TESTREPO) push $(VERBOSE) $(REPO) master~2:refs/heads/master
	git clone $(VERBOSE) $(REPO) tst
	git -C $(TESTREPO) push $(VERBOSE) $(REPO) secrettag
	git -C tst pull $(VERBOSE)
	git -C $(TESTREPO) push $(VERBOSE) $(REPO) master
	git -C tst pull $(VERBOSE)
	git -C $(TESTREPO) push $(VERBOSE) --all $(REPO)
	git -C tst pull $(VERBOSE)
	git ls-remote crypt
	git -C tst ls-remote $(REPO)
	git ls-remote tst
	pylint git-incrypt
	pycodestyle git-incrypt

clean:
	rm -rf man1 crypt tst
