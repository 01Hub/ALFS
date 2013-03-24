# The following code was extracted from Tushar Teredesai's build
# scripts for openoffice at 
# http://www.linuxfromscratch.org/~tushar/openoffice

cd instsetoo/unxlngi4.pro/01/normal
DISPLAY_NUM=0
  while true
    do
      echo "Checking if Display Number $DISPLAY_NUM is available..."
      # Check if there is a lock file
      if [ ! -f /tmp/.X$DISPLAY_NUM-lock ]
	then
	  sleep 10s
	  # Start a Xserver
	  Xvfb :$DISPLAY_NUM >& /dev/null &
	  sleep 10s
	  # Check if server started properly
	  if [ -f /tmp/.X$DISPLAY_NUM-lock ]
	    then
	      break
	  fi
      fi
      DISPLAY_NUM=$(($DISPLAY_NUM+1))
  done
  echo "Using Display Number $DISPLAY_NUM..."
  export DISPLAY=:$DISPLAY_NUM
  ./install --prefix=/opt &&
  kill `cat /tmp/.X$DISPLAY_NUM-lock` >& /dev/null
  unset DISPLAY

