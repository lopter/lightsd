#!/bin/sh

if [ $# != 1 ] ; then
  echo "Usage: $0 unix:///run/lightsd/socket|tcp://[::1]:1234"
  exit 1
fi

url=$1

cat <<'EOF'
### get_light_state ###

{b["label"]: b for b in c.get_light_state("*")["result"]}

#######################
EOF

../../examples/lightsc.py -u ${url}

cat <<'EOF'
### power_toggle / targeting ###

c.power_toggle("*")
c.power_toggle("louis")
c.tag(["louis", "#fosdem"], "fosdem")
c.power_toggle("#fosdem")
c.power_on("*")

################################
EOF

../../examples/lightsc.py -u ${url}

cat <<'EOF'
### set_light_from_hsbk / set_waveform ###

c.set_light_from_hsbk("*", 210, 1, 1, 3500, 1000)
c.set_waveform("*", "TRIANGLE", 285, 0.2, 0.5, 4000, 5000, 3, 0.5, True)
c.set_light_from_hsbk("*", 0, 0, 1, 4500, 1000)
c.set_waveform("*", "SQUARE", 0, 1, 0.5, 4500, 500, 10, 0.5, True)

################################################
EOF

../../examples/lightsc.py -u ${url}
