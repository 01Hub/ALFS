# Begin /etc/profile.d/xterm-titlebars.sh

# The substring match ensures this will work for "xterm" and "xterm-xfree86".
if [ "${TERM:0:5}" = "xterm" ]; then
  PROMPT_COMMAND='echo -ne "\033]0;${USER}@${HOSTNAME} : ${PWD}\007"'
  export PROMPT_COMMAND
fi

# End /etc/profile.d/xterm-titlebars.sh
