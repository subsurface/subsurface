#!/bin/bash
# SPDX-License-Identifier: MIT
# SPDX-Copyright: Copyright (c) 2016 Travis CI GmbH <contact@travis-ci.org>
#
# this is based on code from https://github.com/travis-ci/travis-build

travis_wait() {
  local timeout=$1

  if [[ $timeout =~ ^[0-9]+$ ]]; then
    # looks like an integer, so we assume it is a timeout
    shift
  else
    # default value
    timeout=20
  fi

  local cmd="$@"
  local log_file=travis_wait_$$.log

  $cmd &>$log_file &
  local cmd_pid=$!

  travis_jigger $! $timeout $cmd &
  local jigger_pid=$!
  local result

  {
    wait $cmd_pid 2>/dev/null
    result=$?
    ps -p$jigger_pid &>/dev/null && kill $jigger_pid
  }

  if [ $result -eq 0 ]; then
    echo -e "\nThe command $cmd exited with $result."
  else
    echo -e "\nThe command $cmd exited with $result."
  fi

  echo -e "\nLog:\n"
  cat $log_file

  return $result
}

travis_jigger() {
  # helper method for travis_wait()
  local cmd_pid=$1
  shift
  local timeout=$1 # in minutes
  shift
  local count=0

  # clear the line
  echo -e "\n"

  while [ $count -lt $timeout ]; do
    count=$(($count + 1))
    echo -ne "Still running ($count of $timeout): $@\r"
    sleep 60
  done

  echo -e "\nTimeout (${timeout} minutes) reached. Terminating \"$@\"\n"
  kill -9 $cmd_pid
}
