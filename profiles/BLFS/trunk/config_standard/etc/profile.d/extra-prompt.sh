# Begin /etc/profile.d/extra-prompt.sh

PROMPT_COMMAND="echo -ne '\e[1m${USER}@${HOSTNAME} : ${PWD}\e[0m\a'"
export PROMPT_COMMAND

# End /etc/profile.d/extra-prompt.sh
