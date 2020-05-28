#!/bin/sh

set -eu

# Override this to import from somewhere else, say "../reftable".
SRC=${SRC:-origin}
BRANCH=${BRANCH:-master}

((git --git-dir reftable-repo/.git fetch ${SRC} ${BRANCH}:import && cd reftable-repo && git checkout -f $(git rev-parse import) ) ||
   git clone https://github.com/google/reftable reftable-repo)

cp reftable-repo/c/*.[ch] reftable/
cp reftable-repo/c/include/*.[ch] reftable/
cp reftable-repo/LICENSE reftable/
git --git-dir reftable-repo/.git show --no-patch HEAD \
  > reftable/VERSION

mv reftable/system.h reftable/system.h~
sed 's|if REFTABLE_IN_GITCORE|if 1 /* REFTABLE_IN_GITCORE */|'  < reftable/system.h~ > reftable/system.h

# Remove unittests and compatibility hacks we don't need here.
rm reftable/*_test.c reftable/test_framework.* reftable/compat.*

git add reftable/*.[ch] reftable/LICENSE reftable/VERSION
