#!/bin/bash

# Cloudwatch sink.
# Requires aws cli tools: https://aws.amazon.com/cli/.
# Cloudwatch needs the following things:
#  * Namespace
#  * Metricname
# and optionally
#  * Dimensions.
# These three things are parsed out of the key like that:
#  Namespace:Metricname[:DimensionKey=DimensionValue]*
#
# The dimension InstanceId is use automatically if the command ec2metadata is 
# available. The colon is a special character if this sink is used and should 
# only be used to seperate the fields as described.

command -v aws &> /dev/null
if [[ "$?" != 0 ]]; then
  echo "aws cli missing." 1>&2
  exit 0
fi

OIFS=$IFS
while read line
do
  # line format: key|val|timestamp\n
  if [[ ! -z $line ]]; then

    # splitting it at |
    IFS='|'
    arr=($line); origkey=${arr[0]}; value=${arr[1]}; timestamp=${arr[2]}

    # extracting namespace, metricname and dimensions
    namespace=` echo $origkey | cut -d ':' -f 1`
    key=`       echo $origkey | cut -d ':' -f 2`
    dimensions=`echo $origkey | cut -d ':' -f 3-`

    if [[ -z $namespace || -z $key || -z $timestamp ]]; then
      echo "namespace, key or timestamp missing: $line" 1>&2
      continue
    fi

    # generate json payload
    json="\"MetricName\": \"$key\""
    json="$json, \"Value\": $value"
    json="$json, \"Timestamp\": $timestamp"

    # add dimensions
    IFS=':'

    dimensions_json=""
    # add instance_id to dimensions if available
    command -v ec2metadata &> /dev/null
    if [[ "$?" == 0 ]]; then
      instance_id=`ec2metadata | awk '/instance-id.*/ { print $2 }'`
      dimensions_json="{\"Name\": \"InstanceId\", \"Value\": \"$instance_id\"}"
    fi

    for d in $dimensions; do
      dk=`echo $d | cut -d '=' -f 1`
      dv=`echo $d | cut -d '=' -f 2`
      if [[ ! -z "$dimensions_json" ]]; then dimensions_json+=","; fi
      dimensions_json="$dimensions_json{\"Name\": \"$dk\", \"Value\": \"$dv\"}"
    done

    json="[{$json, \"Dimensions\":[$dimensions_json]}]"

    aws cloudwatch put-metric-data --namespace $namespace --metric-data "$json"
  fi
done 
IFS=$OIFS

exit 0
