#!/bin/bash

CMDBASE="./build/master/optimized/scratch/"
DIRBASE="result/prop-adn/"
PROGRAM="addcn-proportion"
WEIGHTA="0.5"
SIZETNA="5"
SIZETNB="5"
PATHOUT=${WEIGHTA}"_"${SIZETNA}"_"${SIZETNB}
OPTIONS=" --weightTenantA="${WEIGHTA}" --sizeTenantA="${SIZETNA}" --sizeTenantB="${SIZETNB}
COMMAND=${CMDBASE}${PROGRAM}${OPTIONS}" --pathOut="${DIRBASE}${PATHOUT}


while getopts "c:w:a:b:" arg #选项后面的冒号表示该选项需要参数
do
  case $arg in
  #b)
  #  NS3BRANCH=$OPTARG      #参数存在$OPTARG中
  #  ;;
  c)
    PROGRAM=$OPTARG
    ;;
  w)
    WEIGHTA=$OPTARG
    ;;
  a)
    SIZETNA=$OPTARG
    ;;
  b)
    SIZETNB=$OPTARG
    ;;
  ?)  #当有不认识的选项的时候arg为?
    echo "unkonw argument"
    exit 1
    ;;
  esac
done
for i in {1..9}
do
  WEIGHTA="0."${i}
  PATHOUT=${WEIGHTA}"_"${SIZETNA}"_"${SIZETNB}
  mkdir -p ${DIRBASE}${PATHOUT}

  OPTIONS=" --weightTenantA="${WEIGHTA}" --sizeTenantA="${SIZETNA}" --sizeTenantB="${SIZETNB}
  COMMAND=${CMDBASE}${PROGRAM}${OPTIONS}" --pathOut="${DIRBASE}${PATHOUT}
  $COMMAND &
done
