============================================
          Profile BLFS
============================================

This profile is used to build most of the packages
of the BLFS book.

It is expected to be used in the target distribution,
and does not change itself to a chrooted environment.

To be able to have absolute paths to some files in the
BLFS profile, the "runit" command creates a file "files.ent" 
that contains some entity declarations. You should therefore
use "sh ./runit.sh" to execute the profile.

As you could want to vary some configuration files for
different installation, these configuration files can be stored
in various directories. One is provided, names "config_standard"

*** IMPORTANT ***
Before running the profile, you must create a link name 'config'
that points to the configuration directory you choose to use.

The directory config_standard contains the configuration
files corresponding to the book. You can copy it, modify
the files and modify the "config" link so that your files
won't be overwritten when executing the next unpack of the
profile or cvs update.


