#!/usr/bin/env bash

FREQUENCIES=("820" "840" "860" "880" "900" "920")
N_NODES=10

for freq in $FREQUENCIES; do
  bridge=br${freq}
  sudo ip link add $bridge type bridge
  sudo ip link set $bridge up
  for ((ID=0;ID<N_NODES;ID++)); do
    interface=eth${freq}.${ID}
    namespace=ns${interface}
    peer=peer_${interface}
    sudo ip netns add $namespace
    sudo ip link add "$interface" type veth peer name "$peer"
    sudo ip link set "$interface" netns "$namespace"
    sudo ip netns exec "$namespace" ip link set dev "$interface" up
    sudo ip link set dev "$peer" up
    sudo ip link set "$peer" master $bridge
  done
done

