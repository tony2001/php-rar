dnl $Id$
dnl config.m4 for extension rar

PHP_ARG_ENABLE(rar, whether to enable rar support,
[  --enable-rar            Enable rar support])

unrar_sources="unrar/filestr.cpp unrar/scantree.cpp unrar/dll.cpp unrar/qopen.cpp unrar/rar.cpp unrar/strlist.cpp unrar/strfn.cpp unrar/pathfn.cpp unrar/smallfn.cpp unrar/global.cpp unrar/file.cpp unrar/filefn.cpp unrar/filcreat.cpp unrar/archive.cpp unrar/arcread.cpp unrar/unicode.cpp unrar/system.cpp unrar/isnt.cpp unrar/crypt.cpp unrar/crc.cpp unrar/rawread.cpp unrar/encname.cpp unrar/resource.cpp unrar/match.cpp unrar/timefn.cpp unrar/rdwrfn.cpp unrar/consio.cpp unrar/options.cpp unrar/errhnd.cpp unrar/rarvm.cpp unrar/secpassword.cpp unrar/rijndael.cpp unrar/getbits.cpp unrar/sha1.cpp unrar/sha256.cpp unrar/blake2s.cpp unrar/hash.cpp unrar/extinfo.cpp unrar/extract.cpp unrar/extractchunk.cpp unrar/volume.cpp unrar/list.cpp unrar/find.cpp unrar/unpack.cpp unrar/headers.cpp unrar/threadpool.cpp unrar/rs16.cpp unrar/cmddata.cpp unrar/ui.cpp"

if test "$PHP_RAR" != "no"; then
  AC_DEFINE(HAVE_RAR, 1, [Whether you have rar support])
  PHP_SUBST(RAR_SHARED_LIBADD)  
  PHP_REQUIRE_CXX()
  PHP_ADD_LIBRARY_WITH_PATH(stdc++, "", RAR_SHARED_LIBADD)

  PHP_NEW_EXTENSION(rar, rar.c rar_error.c rararch.c rarentry.c rar_stream.c rar_navigation.c $unrar_sources, $ext_shared,,-DRARDLL -DSILENT -Wno-write-strings -Wno-logical-op-parentheses -I@ext_srcdir@/unrar)  
  PHP_ADD_BUILD_DIR($ext_builddir/unrar)  
fi
