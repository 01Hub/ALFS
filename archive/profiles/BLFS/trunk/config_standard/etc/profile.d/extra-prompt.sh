# Begin /etc/profile.d/extra-prompt.sh

PROMPT_COMMAND='echo -ne "\e[1m${USER}@${HOSTNAME} : ${PWD}\e[0m\a"'

# export is disabled as this is an example
# if you want to use PROMPT_COMMAND remember to edit PS1, too 

#export PROMPT_COMMAND

# End /etc/profile.d/extra-prompt.sh
