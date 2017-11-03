CC      = g++
PROF    = 

#Uncomment to compile in Cygwin
#CYGWIN = -DCYGWIN

#Uncomment the line below if you are getting undefined references to dlsym, dlopen, and dlclose.
#Comment it out if you get errors about ldl not being found.
NEED_DL = -ldl

#Some systems need this for dynamic linking to work.
EXPORT_SYMBOLS = -export-dynamic

#Uncomment the line below if you are getting warnings about undefined math functions
NEED_MATH = -lm

#IMC2 - Comment out to disable IMC2 support
IMC = 1

C_FLAGS = -g2 $(W_FLAGS) $(PROF) $(EXPORT_SYMBOLS)
L_FLAGS = $(PROF) $(NEED_MATH) -lz $(NEED_DL)
#D_FLAGS : For the DNS Slave process. No need in linking all the extra libs for this.
D_FLAGS = -g2 -O $(PROF) $(SOLARIS_LINK)

W_FLAGS = -Wall -Werror -Wshadow -Wformat-security -Winline -Wpointer-arith -Wcast-align -Wredundant-decls -Wwrite-strings -Wno-unused-but-set-variable -Wno-bool-compare -Wno-int-in-bool-context -Wno-misleading-indentation -Wno-sequence-point

C_FILES = act_comm.c act_info.c act_move.c act_obj.c act_wiz.c boards.c bounty.c \
	  build.c clans.c color.c comm.c comments.c const.c db.c dns.c fight.c \
	  handler.c hashstr.c hotboot.c interp.c magic.c makeobjs.c mccp.c \
	  misc.c mssp.c mud_comm.c mud_prog.c player.c reset.c save.c sha256.c shops.c \
	  skills.c space.c special.c swskills.c tables.c track.c update.c

ifdef IMC
   C_FILES := imc.c $(C_FILES)
   C_FLAGS := $(C_FLAGS) -DIMC -DIMCSMAUG
endif

O_FILES := $(patsubst %.c,o/%.o,$(C_FILES))

H_FILES = $(wildcard *.h) 

all:
	$(MAKE) -s swreality
	$(MAKE) -s dns

# pull in dependency info for *existing* .o files
-include dependencies.d

ifdef CYGWIN
swreality: $(O_FILES)
	rm -f swreality.exe
	dlltool --export-all --output-def swreality.def $(O_FILES)
	dlltool --dllname swreality.exe --output-exp swreality.exp --def swreality.def
	$(CC) -o swreality.exe $(O_FILES) swreality.exp $(L_FLAGS)
	@echo -e "Generating dependency file ...";
	@$(CC) -MM $(C_FLAGS) $(C_FILES) > dependencies.d
	@perl -pi -e 's.^([a-z]).o/$$1.g' dependencies.d
	@echo -e "Done compiling mud.";
	chmod g+w swreality.exe
	chmod a+x swreality.exe
	chmod g+w $(O_FILES)

clean:
	@rm -f o/*.o swreality.exe dependencies.d resolver.exe resolver.o *~

else
swreality: $(O_FILES)
	rm -f swreality
	$(CC) -export-dynamic -o swreality $(O_FILES) $(L_FLAGS)
	@echo -e "Generating dependency file ...";
	@$(CC) -MM $(C_FLAGS) $(C_FILES) > dependencies.d
	@perl -pi -e 's.^([a-z]).o/$$1.g' dependencies.d
	@echo -e "Done compiling mud.";
	chmod g+w swreality
	chmod a+x swreality
	chmod g+w $(O_FILES)

clean:
	@rm -f o/*.o swreality dependencies.d resolver resolver.o *~
endif

dns: resolver.o
	rm -f resolver
	$(CC) $(D_FLAGS) -o resolver resolver.o
	@echo -e "Done compiling DNS resolver.";
	chmod g+w resolver
	chmod a+x resolver
	chmod g+w resolver.o

.c.o: mud.h
	$(CC) -c $(C_FLAGS) $<

indent:
	indent -ts3 -nut -nsaf -nsai -nsaw -npcs -npsl -ncs -nbc -bls -prs -bap -cbi0 -cli3 -bli0 -l125 -lp -i3 -cdb -c1 -cd1 -sc -pmt $(C_FILES)
	indent -ts3 -nut -nsaf -nsai -nsaw -npcs -npsl -ncs -nbc -bls -prs -bap -cbi0 -cli3 -bli0 -l125 -lp -i3 -cdb -c1 -cd1 -sc -pmt $(H_FILES)

indentclean:
	rm *.c~ *.h~

o/%.o: %.c
	echo "  Compiling $@....";
	$(CC) -c $(C_FLAGS) $< -o $@
