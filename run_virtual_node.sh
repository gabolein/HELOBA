#!/usr/bin/env bash

ID=$1

make build
sudo ip netns exec ns$ID build/program $ID
