cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0048 NEW)
project(fbb VERSION 7.0.12 LANGUAGES C)

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Server)

include(GNUInstallDirs)
set(FBB_USE_SYSTEM_PATHS ON CACHE BOOL "Assume install prefixes are system paths")
# extra install paths we use a lot.
#
# these are best controlled by influencing the CMAKE_INSTALL cache paths, rather than
# their values directly.
if(FBB_USE_SYSTEM_PATHS)
    set(FBB_INSTALL_PKGLIBDIR ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
    set(FBB_INSTALL_FULL_PKGLIBDIR ${CMAKE_INSTALL_FULL_LIBDIR}/${PROJECT_NAME})
    set(FBB_INSTALL_STATEDIR ${CMAKE_INSTALL_LOCALSTATEDIR}/ax25/fbb)
    set(FBB_INSTALL_FULL_STATEDIR ${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/ax25/fbb)
    set(FBB_INSTALL_CONFDIR ${FBB_INSTALL_FULL_CONFDIR})
    set(FBB_INSTALL_FULL_CONFDIR ${CMAKE_INSTALL_FULL_SYSCONFDIR}/ax25/fbb)
else()
    set(FBB_INSTALL_PKGLIBDIR ${CMAKE_INSTALL_LIBDIR})
    set(FBB_INSTALL_FULL_PKGLIBDIR ${CMAKE_INSTALL_FULL_LIBDIR})
    set(FBB_INSTALL_STATEDIR ${CMAKE_INSTALL_LOCALSTATEDIR})
    set(FBB_INSTALL_FULL_STATEDIR ${CMAKE_INSTALL_FULL_LOCALSTATEDIR})
    set(FBB_INSTALL_CONFDIR ${CMAKE_INSTALL_SYSCONFDIR})
    set(FBB_INSTALL_FULL_CONFDIR ${CMAKE_INSTALL_FULL_SYSCONFDIR})
endif()

find_library(MATH_LIBRARY m)
find_library(AX25_LIBRARY ax25 REQUIRED)
find_package(Curses)
find_package(X11 COMPONENTS Xext Xpm Xt)
find_package(Motif)

if(MATH_LIBRARY)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${MATH_LIBRARY})
endif()
# hack around the fact that *everything* references this directory.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}/include)

# generate config.h
configure_file(include/config.h.in include/config.h @ONLY)
# generate the fbb_conf.c file
configure_file(src/fbb_conf.c.in fbb_conf.c @ONLY)

# common (static) library to most of these tools
add_library(libxfbbd STATIC
        src/arbre.c
        src/autobin.c
        src/balise.c
        src/bidexms.c
        src/conf.c
        src/console.c
        src/date.c
        src/devio.c
        src/dos_1.c
        src/dos_dir.c
        src/driver.c
        src/drv_aea.c
        src/drv_ded.c
        src/drv_hst.c
        src/drv_kam.c
        src/drv_mod.c
        src/drv_pop.c
        src/drv_sock.c
        src/drv_tcp.c
        src/edit.c
        src/ems.c
        src/error.c
        src/exec_pg.c
        fbb_conf.c
        src/fbb_orb.c
        src/fortify.c
        src/forward.c
        src/fwdovl1.c
        src/fwdovl2.c
        src/fwdovl3.c
        src/fwdovl4.c
        src/fwdovl5.c
        src/fwdovl6.c
        src/fwdovl7.c
        src/fwdutil.c
        src/gesfic.c
        src/ibm.c
        src/info.c
        src/init.c
        src/init_srv.c
        src/init_tnc.c
        src/initfwd.c
        src/initport.c
        src/k_tasks.c
        src/kernel.c
        src/lzhuf.c
        src/maint_fw.c
        src/mbl_edit.c
        src/mbl_expo.c
        src/mbl_impo.c
        src/mbl_kill.c
        src/mbl_lc.c
        src/mbl_list.c
        src/mbl_log.c
        src/mbl_menu.c
        src/mbl_opt.c
        src/mbl_prn.c
        src/mbl_read.c
        src/mbl_rev.c
        src/mbl_stat.c
        src/mbl_sys.c
        src/mbl_user.c
        src/mblutil.c
        src/md5c.c
        src/modem.c
        src/nomenc.c
        src/nouvfwd.c
        src/pac_crc.c
        src/pacsat.c
        src/qraloc.c
        src/redist.c
        src/rx25.c
        src/serv.c
        src/serveur.c
        src/statis.c
        src/sysinfo.c
        src/themes.c
        src/tnc.c
        src/tncio.c
        src/trait.c
        src/trajec.c
        src/trajovl.c
        src/variable.c
        src/warning.c
        src/watchdog.c
        src/waveplay.c
        src/wp.c
        src/wp_mess.c
        src/wpserv.c
        src/xfwd.c
        src/xmodem.c
        src/yapp.c
)
target_include_directories(libxfbbd PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

add_executable(xfbbd
        src/xfbbd.c)
target_compile_definitions(xfbbd PRIVATE __ORB__)
target_link_libraries(xfbbd PRIVATE libxfbbd ${EXTRA_LIBS} ${AX25_LIBRARY})
install(TARGETS xfbbd RUNTIME
        DESTINATION ${CMAKE_INSTALL_SBINDIR})

add_executable(clr_user
    src/clr_user.c
    fbb_conf.c)
install(TARGETS clr_user RUNTIME
    DESTINATION ${FBB_INSTALL_PKGLIBDIR}/tool)

add_executable(ajoursat
        src/ajoursat.c)
install(TARGETS ajoursat RUNTIME
        DESTINATION ${CMAKE_INSTALL_SBINDIR})

add_executable(satdoc
    src/satdoc.c)
install(TARGETS satdoc RUNTIME
        DESTINATION ${CMAKE_INSTALL_SBINDIR})

add_executable(satupdat
    src/satupdat.c)
install(TARGETS satupdat RUNTIME
        DESTINATION ${CMAKE_INSTALL_SBINDIR})

add_executable(reqdir
    src/reqdir.c
    fbb_conf.c)
install(TARGETS reqdir RUNTIME
        DESTINATION ${FBB_INSTALL_PKGLIBDIR}/server)

add_executable(epurwp
    src/epurwp.c
    fbb_conf.c)
install(TARGETS epurwp RUNTIME
        DESTINATION ${FBB_INSTALL_PKGLIBDIR}/tool)

add_executable(epurmess
    src/epurmess.c
    fbb_conf.c)
install(TARGETS epurmess RUNTIME
        DESTINATION ${FBB_INSTALL_PKGLIBDIR}/tool)


add_executable(fbbgetconf
    src/fbbgetconf.c
    fbb_conf.c)

add_executable(maintinf
    src/maintinf.c
    fbb_conf.c)
install(TARGETS maintinf RUNTIME
        DESTINATION ${FBB_INSTALL_PKGLIBDIR}/tool)


if(CURSES_FOUND)
    add_executable(xfbbC
        src/md5c.c
        src/terminal.c
        src/xfbbC.c)
    target_include_directories(xfbbC PRIVATE ${CURSES_INCLUDE_DIRS})
    target_link_libraries(xfbbC PRIVATE ${CURSES_LIBRARIES} ${EXTRA_LIBS})
    install(TARGETS xfbbC RUNTIME
            DESTINATION ${CMAKE_INSTALL_SBINDIR})
endif()

if(X11_FOUND AND MOTIF_FOUND)
    add_executable(xfbb
        src/X11/xfbbabtd.c
        src/X11/xfbbcnsl.c
        src/X11/xfbbedtm.c
        src/X11/xfbbedtu.c
        src/X11/xfbblcnx.c
        src/X11/xfbbmain.c
        src/X11/xfbbpndd.c
        src/X11/xeditor.c
        src/X11/xfbb.c)
    target_include_directories(xfbb
        PRIVATE
        ${MOTIF_INCLUDE_DIRS})
    target_link_libraries(xfbb PRIVATE
        libxfbbd
        X11::X11 X11::Xext X11::Xpm X11::Xt
        ${MOTIF_LIBRARIES} ${AX25_LIBRARY} ${EXTRA_LIBS})

    install(TARGETS xfbb RUNTIME
            DESTINATION ${CMAKE_INSTALL_SBINDIR})

    add_executable(xfbbX_cl
        src/md5c.c
        src/X11/xfbbXabtd.c
        src/X11/xfbbXcnsl.c
        src/X11/xfbbX.c)
    target_include_directories(xfbbX_cl
            PRIVATE
            ${MOTIF_INCLUDE_DIRS})
    target_link_libraries(xfbbX_cl PRIVATE
            X11::X11 X11::Xext X11::Xpm X11::Xt
            ${MOTIF_LIBRARIES} ${EXTRA_LIBS})

    install(TARGETS xfbbX_cl RUNTIME
            DESTINATION ${CMAKE_INSTALL_SBINDIR})
    install(PROGRAMS scripts/xfbbX
            TYPE SBIN)
endif()

# install scripts
configure_file(scripts/20_epurmess.in 20_epurmess @ONLY)
configure_file(scripts/20_epursat.in 20_epursat @ONLY)
configure_file(scripts/20_epurwp.in 20_epurwp @ONLY)
configure_file(scripts/fbb.in fbb @ONLY)
install(PROGRAMS
        ${CMAKE_CURRENT_BINARY_DIR}/20_epurmess
        ${CMAKE_CURRENT_BINARY_DIR}/20_epursat
        ${CMAKE_CURRENT_BINARY_DIR}/20_epurwp
        DESTINATION ${FBB_INSTALL_PKGLIBDIR}/script/maintenance)
install(PROGRAMS
        ${CMAKE_CURRENT_BINARY_DIR}/fbb
        TYPE SBIN)

# install the configuration files
configure_file(etc/fbbopt.conf.in fbbopt.conf @ONLY)
configure_file(etc/port.sys.sample.in port.sys.sample @ONLY)
configure_file(etc/protect.sys.in protect.sys @ONLY)
configure_file(etc/amsat.fwd.in amsat.fwd @ONLY)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/fbbopt.conf
    ${CMAKE_CURRENT_BINARY_DIR}/port.sys.sample
    ${CMAKE_CURRENT_BINARY_DIR}/protect.sys
    DESTINATION ${FBB_INSTALL_CONFDIR}
    COMPONENT NewConfig
    EXCLUDE_FROM_ALL)
install(FILES
    etc/bbs.sys
    etc/cron.sys
    etc/forward.sys
    etc/langue.sys
    etc/passwd.sys
    etc/redist.sys
    etc/reject.sys
    etc/swapp.sys
    etc/themes.sys
    DESTINATION ${FBB_INSTALL_CONFDIR}
    COMPONENT NewConfig
    EXCLUDE_FROM_ALL)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/amsat.fwd
    DESTINATION ${FBB_INSTALL_CONFDIR}/fwd)
install(DIRECTORY
    etc/lang/
    DESTINATION ${FBB_INSTALL_CONFDIR}/lang)

# create empty directories
install(DIRECTORY DESTINATION ${FBB_INSTALL_PKGLIBDIR})
foreach(SUBDIR
    filter pg server tool script script/error_return script/end_session
    script/rerun script/maintenance script/stop_system script/sig_segv script/reset)
    install(DIRECTORY DESTINATION ${FBB_INSTALL_PKGLIBDIR}/${SUBDIR})
endforeach()

install(DIRECTORY DESTINATION ${FBB_INSTALL_STATEDIR})
foreach(SUBDIR
        log oldmail sat wp fbbdos fbbdos/yapp fbbdos/readonly mail binmail)
    install(DIRECTORY DESTINATION ${FBB_INSTALL_STATEDIR}/${SUBDIR})
endforeach()
foreach(MAIL_SUBDIR RANGE 0 9)
    install(DIRECTORY DESTINATION ${FBB_INSTALL_STATEDIR}/mail/mail${MAIL_SUBDIR})
    install(DIRECTORY DESTINATION ${FBB_INSTALL_STATEDIR}/mail/binmail${MAIL_SUBDIR})
endforeach()

configure_file(doc/fbb.conf.min.sample.in doc/fbb.conf.min.sample @ONLY)
configure_file(doc/fbb.conf.sample.in doc/fbb.conf.sample @ONLY)
configure_file(doc/fbb.sh.in doc/fbb.sh @ONLY)
install(FILES doc/satdoc-e.doc doc/satdoc-f.doc doc/satupdat.doc
        ${CMAKE_CURRENT_BINARY_DIR}/doc/fbb.conf.min.sample
        ${CMAKE_CURRENT_BINARY_DIR}/doc/fbb.conf.sample
        ${CMAKE_CURRENT_BINARY_DIR}/doc/fbb.sh
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
        COMPONENT Documentation)
install(DIRECTORY doc/html/
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/html
    COMPONENT Documentation)
install(FILES man/fbb.8 man/xfbbC.8 man/xfbbd.8
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man8
        COMPONENT Documentation)
install(FILES man/fbbgetconf.1
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
        COMPONENT Documentation)