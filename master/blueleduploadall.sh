#upload firware to all ports
while read p; do
    ./upload $p 
done </etc/blueled/portlist 
