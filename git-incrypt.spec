Name: git-incrypt
Version: 0.9.0
Release: 1
Summary: A git remote helper to encrypt git repositories incrementally
License: GPL-2.0
Source: git-incrypt-%{version}.tar.xz
BuildArch: noarch
Requires: git-core
Requires: python3
Requires: gpg
Requires: python-pygit2
BuildRequires: asciidoc
BuildRequires: xmlto
BuildRequires: git-core
#BuildRequires: gpg
#BuildRequires: python-pygit2
#BuildRequires: man

%description
A git remote helper to encrypt git repositories incrementally

%prep
%autosetup -p1

%build
%make_build

%define gitexecdir %(git --exec-path)

%check
#git config --global user.email "testuser@example.com"
#git config --global user.name "Test User"
#gpg --batch --gen-key .github/workflows/testkey.conf
#make KEY:=$(gpg --list-keys --with-colons | awk -F: '/^pub/ {print $5; exit}') test

%install
make DESTDIR:="%{buildroot}" DOCDIR:=%{_docdir} MANDIR:=%{_mandir} LICENSEDIR:=/usr/share/licenses install

%files
%license COPYING
%{gitexecdir}/git-incrypt
%{gitexecdir}/git-remote-incrypt
%{_docdir}/%{name}
%{_mandir}/man1/git-incrypt.1*
