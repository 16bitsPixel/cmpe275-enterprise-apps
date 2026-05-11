#!/bin/bash
set -e

if [ "$#" -ne 2 ]; then
  echo "Usage: ./setup_demo_ips_2laptops.sh <LAPTOP1_IP> <LAPTOP2_IP>"
  exit 1
fi

L1="$1"
L2="$2"

rm -rf basic/config
cp -r basic/config_templates basic/config

find basic/config -name "*.conf" -type f -exec sed -i '' \
  -e "s/__LAPTOP1_IP__/$L1/g" \
  -e "s/__LAPTOP2_IP__/$L2/g" {} \;

echo "Updated configs:"
grep -R "neighbors=" basic/config