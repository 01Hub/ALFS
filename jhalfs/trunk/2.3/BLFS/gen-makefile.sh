#!/bin/bash
#
# $Id$
#
set -e

# TEMPORARY VARIABLES.. development use only
declare MKFILE=Makefile
declare PREV_PACKAGE=""
declare BUILD_SCRIPTS=scripts
declare TRACKING_DIR=tracking-dir

HEADER="# This file is automatically generated by gen-makefile.sh
# YOU MAY NEED TO EDIT THIS FILE MANUALLY
#
# Generated on `date \"+%F %X %Z\"`"


#----------------------------------#
__wrt_target() {                   # Create target and initialize log file
#----------------------------------#
  local i=$1
  local PREV=$2
(
cat << EOF

$i:  $PREV
	@\$(call echo_message, Building)
	@/bin/bash progress_bar.sh \$@ \$\$PPID &
EOF
) >> $MKFILE.tmp
}



#----------------------------------#
__write_build_cmd() {              #
#----------------------------------#
(
cat << EOF
	@source ../envars.conf && ${BUILD_SCRIPTS}/\$@ >logs/\$@ 2>&1
EOF
) >> $MKFILE.tmp
}

#----------------------------------#
__wrt_touch() {                    #
#----------------------------------#
  local pkg_name=$1
  local pkg_ver=$2
  local alsa_ver=$(grep "^alsa[[:space:]]" ../packages | cut -f3)
  local kde_core_ver=$(grep "^kde-core[[:space:]]" ../packages | cut -f3)
  local xorg7_ver=$(grep "^xorg7[[:space:]]" ../packages | cut -f3)

  if [[ -n "$pkg_ver" ]] ; then
(
cat << EOF
	@rm -f \$(TRACKING_DIR)/${pkg_name#*-?-}-{0..9}* && \\
	touch \$(TRACKING_DIR)/${pkg_name#*-?-}-${pkg_ver}
EOF
) >> $MKFILE.tmp
  fi

  case $pkg_name in
    *-alsa-lib ) #this the unique mandatory package for ALSA support.
(
cat << EOF
	@rm -f \$(TRACKING_DIR)/alsa-{0..9}* && \\
	touch \$(TRACKING_DIR)/alsa-${alsa_ver}
EOF
) >> $MKFILE.tmp
      ;;
    *-kdebase )
(
cat << EOF
	@rm -f \$(TRACKING_DIR)/kde-core-{0..9}* && \\
	touch \$(TRACKING_DIR)/kde-core-${kde_core_ver}
EOF
) >> $MKFILE.tmp
      ;;
    *-xorg7-driver ) # xtrerm2 and rman are optional
(
cat << EOF
	@rm -f \$(TRACKING_DIR)/xorg7-{0..9}* && \\
	touch \$(TRACKING_DIR)/xorg7-${xorg7_ver}
EOF
) >> $MKFILE.tmp
      ;;
  esac

(
cat << EOF
	@touch  \$@ && \\
	sleep .25 && \\
	echo -e "\n\n "\$(BOLD)Target \$(BLUE)\$@ \$(BOLD)OK && \\
	echo --------------------------------------------------------------------------------\$(WHITE)
EOF
) >> $MKFILE.tmp
}


#----------------------------#
__write_entry() {            #
#----------------------------#
  local script_name=$1
  local pkg_ver=$2

  echo -n "${tab_}${tab_} entry for <$script_name>"

  #--------------------------------------------------------------------#
  #         >>>>>>>> START BUILDING A Makefile ENTRY <<<<<<<<          #
  #--------------------------------------------------------------------#
  #
  # Drop in the name of the target on a new line, and the previous target
  # as a dependency. Also call the echo_message function.
  __wrt_target "${script_name}" "$PREV_PACKAGE"
  __write_build_cmd

  # Include a touch of the target name so make can check
  # if it's already been made.
  __wrt_touch "${script_name}" "${pkg_ver}"
  #
  #--------------------------------------------------------------------#
  #              >>>>>>>> END OF Makefile ENTRY <<<<<<<<               #
  #--------------------------------------------------------------------#
  echo " .. OK"
}

#----------------------------#
__write_meta_pkg_touch() {   #
#----------------------------#
  local meta_pkg=$1
  local pkg_ver=$(grep "^${meta_pkg}[[:space:]]" ../packages | cut -f3)
  local gnome_core_ver=$(grep "^gnome-core[[:space:]]" ../packages | cut -f3)
  local kde_full_ver=$(grep "^kde-full[[:space:]]" ../packages | cut -f3)

(
cat << EOF

999-z-$meta_pkg:  $PREV_PACKAGE
	@rm -f \$(TRACKING_DIR)/${meta_pkg}-{0..9}* && \\
	touch \$(TRACKING_DIR)/${meta_pkg}-${pkg_ver}
EOF
) >> $MKFILE.tmp

  case $meta_pkg in
    gnome-full )
(
cat << EOF
	@rm -f \$(TRACKING_DIR)/gnome-core-{0..9}* && \\
	touch \$(TRACKING_DIR)/gnome-core-${gnome_core_ver}
EOF
) >> $MKFILE.tmp
      ;;
    kde-koffice )
(
cat << EOF
	@rm -f \$(TRACKING_DIR)/kde-full-{0..9}* && \\
	touch \$(TRACKING_DIR)/kde-full-${kde_full_ver}
EOF
) >> $MKFILE.tmp
      ;;
  esac

(
cat << EOF
	@touch  \$@
EOF
) >> $MKFILE.tmp

}

#----------------------------#
generate_Makefile () {       #
#----------------------------#


  echo "${tab_}Creating Makefile... ${BOLD}START${OFF}"

  # Start with a clean files
  >$MKFILE
  >$MKFILE.tmp


  for package_script in scripts/* ; do
    this_script=`basename $package_script`
    pkg_ver=$(grep "^${this_script#*-?-}[[:space:]]" ../packages | cut -f3)
    pkg_list="$pkg_list ${this_script}"
    __write_entry "${this_script}" "${pkg_ver}"
    PREV_PACKAGE=${this_script}
  done

  PACKAGE=$(basename $PWD)

   # alsa, kde-core and xorg7 are also available dependencies, thus handled
   # in another way.
  case $PACKAGE in
    gnome-core | \
    gnome-full | \
    kde-full | \
    kde-koffice )  pkg_list="$pkg_list 999-z-${PACKAGE}"
                  __write_meta_pkg_touch "${PACKAGE}"
                  ;;
  esac


  # Add a header, some variables and include the function file
  # to the top of the real Makefile.
(
    cat << EOF
$HEADER

PACKAGE= $PACKAGE
TRACKING_DIR= $TRACKING_DIR

BOLD= "[0;1m"
RED= "[1;31m"
GREEN= "[0;32m"
ORANGE= "[0;33m"
BLUE= "[1;34m"
WHITE= "[00m"

define echo_message
  @echo \$(BOLD)
  @echo --------------------------------------------------------------------------------
  @echo \$(BOLD)\$(1) target \$(BLUE)\$@\$(BOLD)
  @echo \$(WHITE)
endef


define fin_message
  @echo \$(BOLD)
  @echo --------------------------------------------------------------------------------
  @echo \$(BOLD) Build complete for the package \$(BLUE)\$(PACKAGE)\$(BOLD) and its dependencies
  @echo \$(WHITE)
endef

all : $pkg_list
	@\$(call fin_message )
EOF
) > $MKFILE

  cat $MKFILE.tmp >> $MKFILE
  echo "${tab_}Creating Makefile... ${BOLD}DONE${OFF}"

  rm $MKFILE.tmp

}

if [[ -e Config.in ]] ; then
  echo -e "\n\tThis script must be run from inside a target package directory.\n"
  exit 1
fi

if [[ ! -d scripts ]] ; then
  echo -e "\n\tNo ./scripts/ directory has been found.\n"
  exit 1
fi

generate_Makefile

cp ../progress_bar.sh .

mkdir -p logs
