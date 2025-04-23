OBJECTS += incrypt.o

MAN1_TXT += git-incrypt.adoc

git-remote-incrypt$X: incrypt.o GIT-LDFLAGS $(GITLIBS)
	$(QUIET_LINK)$(CC) $(ALL_CFLAGS) -o $@ $(ALL_LDFLAGS) $(filter %.o,$^) $(LIBS)
