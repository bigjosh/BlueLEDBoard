# kill all of the connected and running boards 

while read p; do
  echo checking $p 
  if [ -e /tmp/blueled/$p/pid ]; then
    echo killing $(cat /tmp/blueled/$p/pid)
    kill $(cat /tmp/blueled/$p/pid)
  fi    
done </etc/blueled/portlist 
