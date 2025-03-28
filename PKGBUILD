# Maintainer: Robert Schiele <rschiele@gmail.com>

pkgname=git-incrypt
pkgver=0.9.0
pkgrel=1
pkgdesc="git-incrypt: A git remote helper to encrypt git repositories incrementally"
arch=('i686' 'x86_64')
url="https://github.com/schiele/git-incrypt/"
license=('GPL2')
depends=('git' 'python-pygit2')
makedepends=('asciidoc' 'xmlto')
source=("COPYING" "FORMAT.md" "Makefile" "README.md" "asciidoc.conf" "git-incrypt" "git-incrypt.adoc" "manpage-bold-literal.xsl" "manpage-normal.xsl")
sha256sums=('SKIP' 'SKIP' 'SKIP' 'SKIP' 'SKIP' 'SKIP' 'SKIP' 'SKIP' 'SKIP')

build() {
	make clean
	make
	gzip man1/git-incrypt.1
}

package() {
	mkdir -p "$pkgdir"/usr/lib/git-core
	install -m 755 git-incrypt "$pkgdir"/usr/lib/git-core/
	ln -s git-incrypt "$pkgdir"/usr/lib/git-core/git-remote-incrypt
	mkdir -p "$pkgdir"/usr/share/licenses/$pkgname
	install -m 644 COPYING "$pkgdir"/usr/share/licenses/$pkgname/
	mkdir -p "$pkgdir"/usr/share/doc/$pkgname
	install -m 644 FORMAT.md "$pkgdir"/usr/share/doc/$pkgname/
	install -m 644 README.md "$pkgdir"/usr/share/doc/$pkgname/
	mkdir -p "$pkgdir"/usr/share/man/man1
	install -m 644 man1/git-incrypt.1.gz "$pkgdir"/usr/share/man/man1/
}
