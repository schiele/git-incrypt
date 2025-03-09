# git-incrypt

This is a git remote helper that allows for full transparent encryption of git
repositories on unsecure remote locations.

## Experimental Disclaimer Warning

This is still experimental. Encryption format is still not stable, which might
require you to re-encrypt your data if you update to a future version.

So use at your own risk and expect some weird errors. Because of this state
the installation is also a bit more nerdy than what you might expect.

## Issues and Contributions

If you see issues with the installation or usage, feel free to open an issue
at https://github.com/schiele/git-incrypt/issues. Be aware that this is a
hobby project of mine, so read the issues from other people first that might
be related to your problem, be polite, and provide all information necessary
to properly understand your problem, including what you intended to do, what
command you invoked and what was the output the tool produced.

If you have suggestions on how to improve things, feel free to provide them as
well. Optimally you provide even a pull request but just opening an issue is
also welcome.

## Installation

To make this work you first need to install pygit2. Either use your
distribution's package manager or alternatively invoke:

```
pip install pygit2
```

Next you either need to put this directoy inside your system search path or
link or copy the files `git-incrypt` and `git-remote-incrypt` to some
directory that is in the system search path.

I recommend you configure GPG to use an agent and it is likely not a good idea
to receive password prompts during git operations.

## Usage

To create a new encrypted repository you first create an empty directory.

Inside this directory you invoke the command:

```
git incrypt init $GPG_KEY_ID
```

where `$GPG_KEY_ID` is one or multiple GPG key IDs to be used to encrypt the
data in the repository. Note that this version does not yet have a command to
change the list of keys later, so give it a thought before setting up the
repository. However if you need to change keys you can always delete the
encrypted repository later and push your stuff into a new encrypted repository
with other keys. For sure we should have a command in future versions that an
re-encrypt the data with new keys in the future.

The resulting repository can then be pushed to a remote location with the
command:

```
git push --mirror $REMOTE_URL
```

where `$REMOTE_URL` is the URL of the remote repository you want to push the
encrypted content to.

After that you can remove the local copy of the encrypted repository. It is no
longer needed.

From now on you can just use the regular git commands to communicate with the
encrypted remote repository. They all should work in the ususal way but
whenever you need to supply the remote URL to the encrypted repository you
need to prefix it with `incrypt::`, for example when your remote storage place
is:

```
git@github.com:schiele/git-incrypt-crypted.git
```

you provide instead:

```
incrypt::git@github.com:schiele/git-incrypt-crypted.git
```

For a fresh clone of the encrypted repository this would be for example:

```
git clone incrypt::git@github.com:schiele/git-incrypt-crypted.git
```

To add an encrypted remote to an existing repository this would be something
like:

```
git remote add crypted incrypt::git@github.com:schiele/git-incrypt-crypted.git
```

Within the repositories you can use commands like `git pull`, `git fetch`, or
`git push` without special considerations.

## Concept and Discussion

A sketch of the repository format can be found in [`FORMAT.md`](FORMAT.md).

The approach we take here is to completely encrypt every single git object.
With this approach no filters need to get applied as with
https://www.agwa.name/projects/git-crypt/ but all action happens when fetching
or pushing data to the remote.

There is https://github.com/spwhitton/git-remote-gcrypt that operate as a
remote helper, similar to this one but they compress the complete pack files,
making incremental changes on arbitrary git URLs expensive since this
potentially requires re-transmitting large pack files.

Our approach has advantages and disadvantages compared to those other
approaches:

Compared with the first project above we do not need any particular filter
configuration we could get wrong and might leak data by accident. For sure
this comes with the downside that we either encrypt the whole repository or we
don't. There is no partial encryption possible. On the other hand our approach
allows pushing the same git repository to an unencrypted remote in a secure
location, like a local server and duplicate the same on an encrypted remote in
public space to exchange the code with other trusted parties.

Compared with the second approach we can perform incremental changes to the
git repository in a more efficient way. If there is only a small change on the
unencrypted repository, this causes only a small change to the encrypted
repository, while the other approach might require large transfers of the pack
files changed. On the other side our approach does reveal more information
about the repository structure in the sense that a potential attacker can
count the amount of branches or tags that do exist and can count the amount of
commits in each branch and their structural relationships to each other. An
attacker could not see who created them or when they were created.

Since we encrypt each object separately we also pay a price by a space
increase of the overall repository of a bit less than a ten-fold increase.
This is caused by the fact that the delta algorithm in git can no longer
detect similarities between the individual encrypted objects. Since in a
typical workflow changes to the repository are of small increments though the
speed increase is likely worth it. Also the initial encryption of an already
very large repository can take quite some time. Encrypting the complete source
repository of git takes about 45 minutes on my current laptop. By setting the
variable `selfcontained` to `False` in the code this time goes down to only 10
minutes. If you do so though the repository will no longer work with shallow
clones in the future when we implement this. Therefore I left the
self-contained version as a default since the performance penalty seems only
relevant for the expensive initial encryption. --- And after all most users
will have significantly smaller repositories compared to the repository of git
itself.

The way we encrypt individual objects might reveal data that an attacker could
use to undermine the encryption, though I tried to mitigate the risks wherever
I saw one. A more detailed analysis and discussion should follow in the future
in this document. Should you identify a problem, I am for sure interested in
learning about it.

At the moment the implementation is still quite experimental with bad error
handling and a lot of technical debt in the code.

But you are free to already look around here and play with the tool. Further
documentation will follow.

You can also find an encrypted version of this repository at
https://github.com/schiele/git-incrypt-crypted/ for you to see how things look
like on the ecrypted side.
