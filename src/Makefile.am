## Process this file with automake to produce Makefile.in

# set the include path found by configure
gpicview_CPPFLAGS = $(all_includes) \
	$(X11_CFLAGS) \
        $(GTK_CFLAGS) \
        $(ADDITIONAL_FLAGS) \
	-I$(top_srcdir) \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

bin_PROGRAMS = gpicview
gpicview_SOURCES = \
	gpicview.c \
	image-list.c \
	View/main-win.c \
	View/image-view.c \
	working-area.c \
	ptk-menu.c \
	View/file-dlgs.c \
	Imaging/jpeg-tran.c \
	Imaging/tran-supp.c \
	pref.c \
	Imaging/jhead.c \
	Imaging/exif.c \
	Imaging/jpgfile.c \
	ViewModel/main-win-vm.c \
	ViewModel/image-item.c

# the library search path.
# use -nodefaultlibs to prevent linking libstdc++
gpicview_LDADD = $(X11_LIBS) $(GTK_LIBS) -lm

noinst_HEADERS = \
	glib-mem.h \
	image-list.h \
	View/main-win.h \
	View/image-view.h \
	working-area.h \
	ptk-menu.h \
	View/file-dlgs.h \
	Imaging/jpeg-tran.h \
	Imaging/tran-supp.h \
	pref.h \
	Imaging/jhead.h \
	ViewModel/main-win-vm.h \
	ViewModel/image-item.h

noinst_PROGRAMS=xml-purge
xml_purge_SOURCES=xml-purge.c
xml_purge_CFLAGS=@X11_CFLAGS@
xml_purge_LDADD=@X11_LIBS@

