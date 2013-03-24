# This will tinker with the value of TERM in order to convince certain apps
# that we can, indeed, display color in their window.
 
if [ -n "$COLORTERM" ]; then
  export TERM=xterm-color
fi
 
if [ "$TERM" = "xterm" ]; then
  export TERM=xterm-color
fi
