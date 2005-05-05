if [ ! -e /etc/xml/catalog ]; then mkdir -p /etc/xml; xmlcatalog \
    --noout --create /etc/xml/catalog; fi &&
if [ ! -e /etc/xml/docbook ]; then xmlcatalog --noout --create \
    /etc/xml/docbook; fi
