#!/bin/sh

set -e

manifest=$1
revision=$2
range=$3

remote() {
  name=$1
  url=$2

  if ! git remote -v | grep -qE ^$name; then
    git remote add --fetch $name $url
  fi
}

import() {
  git restore --pathspec-from-file $manifest --source $revision
  git restore .
  git add .
  git commit -m "Import from mainline $revision"
}

apply() {
    git cherry-pick \
      --strategy recursive \
      --strategy-option no-renames \
      --first-parent \
      --no-merges \
      --invert-grep --grep REVERTME --grep DEBUG \
      $range
}

remote mainline git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
remote stable git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
remote dfl https://github.com/OPAE/linux-dfl.git

import
apply
