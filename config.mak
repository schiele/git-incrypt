CFLAGS += -fPIC

OBJECTS += incrypt-plugin.o

MAN1_TXT += git-incrypt.adoc

incrypt-plugin.so: incrypt-plugin.o GIT-LDFLAGS $(GITLIBS)
	$(QUIET_LINK)$(CC) $(ALL_CFLAGS) -o $@ $(ALL_LDFLAGS) -shared $(filter %.o,$^) $(LIBS) $(LIB_4_CRYPTO)
