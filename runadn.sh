#!/bin/bash

CMDBASE="./build/master/optimized/scratch/"
OPTBASE=" --enableADN --writeResult --writeFlowInfo "
DIRBASE="result/adn-spin/"
PROGRAM="adn-spin"
PATHOUT="ds"
OPTIONS="--enableDS"
COMMAND=${CMDBASE}${PROGRAM}${OPTBASE}${OPTIONS}" --pathOut="${DIRBASE}${PATHOUT}


while getopts "c:d:o:" arg #选项后面的冒号表示该选项需要参数
do
  case $arg in
  #b)
  #  NS3BRANCH=$OPTARG      #参数存在$OPTARG中
  #  ;;
  c)
    PROGRAM=$OPTARG
    ;;
  d)
    PATHOUT=$OPTARG
    ;;
  o)
    OPTIONS=$OPTARG
    ;;
  ?)  #当有不认识的选项的时候arg为?
    echo "unkonw argument"
    exit 1
    ;;
  esac
done
mkdir -p ${DIRBASE}${PATHOUT}
for i in {1..9}
do
  COMMAND=${CMDBASE}${PROGRAM}${OPTBASE}${OPTIONS}" --pathOut="${DIRBASE}${PATHOUT}" --miceLoad=0."${i}
  $COMMAND &
done
