# git-incrypt

This is a git remote helper that allows for full transparent encryption of git
repositories on unsecure remote locations.

The approach we take here is to completely encrypt every single git object.
With this approach no filters need to get applied as with
https://www.agwa.name/projects/git-crypt/ but all action happens when fetching
or pushing data to the remote.

Another project I have seen before (but currently couldn't find anymore) did
operate as a remote helper, similar to this one but they did compress the
complete pack files, making incremental changes expensive since this
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
git repository in a more efficient way since if there is only a small change
on the unencrypted repository, this causes only a small change to the
encrypted repository, while the other approach might require large transfers
if the pack files changed. On the other side our approach does reveal more
information about the repository structure in the sence that a potential
attacker can count the amount of branches or tags that do exist and can count
the amount of commits in each branch and their structural relationships to
each other. The attacker could not see who created them or when they were
created.

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
