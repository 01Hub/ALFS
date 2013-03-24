# Begin /etc/profile.d/jdk.sh

# Set JAVA_HOME directory
JAVA_HOME=/opt/jdk/jdk
export JAVA_HOME

# Adjust PATH
pathappend ${JAVA_HOME}/bin PATH

# Auto Java Classpath Updating
# Create symlinks to this directory for auto classpath setting
AUTO_CLASSPATH_DIR=/usr/lib/classpath
if [ -z ${CLASSPATH} ]; then
        CLASSPATH=.:${AUTO_CLASSPATH_DIR}
else
        CLASSPATH="${CLASSPATH}:.:${AUTO_CLASSPATH_DIR}"
fi

# Check for empty AUTO_CLASSPATH_DIR
ls ${AUTO_CLASSPATH_DIR}/*.jar &> /dev/null &&
for i in ${AUTO_CLASSPATH_DIR}/*.jar
        do CLASSPATH=${CLASSPATH}:"${i}"
done
export CLASSPATH

# End /etc/profile.d/jdk.sh
