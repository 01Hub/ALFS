ENV_PATH=`grep '^ENV_PATH' /etc/login.defs.orig | \
    awk '{ print $2 }' | sed 's/PATH=//'` &&
echo 'PATH        DEFAULT='`echo "${ENV_PATH}"`'        OVERRIDE=${PATH}' \
    >> /etc/security/pam_env.conf &&
unset ENV_PATH
