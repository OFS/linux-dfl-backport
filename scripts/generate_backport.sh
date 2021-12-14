#!/bin/sh
set -x

cd ..
if [ ! -d stable.git ]; then
    git clone --bare git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git stable.git
fi
if [ ! -d stable ]; then
   git clone stable.git
fi

if [ ! -d linux-dfl.git ]; then
    git clone --bare https://github.com/OPAE/linux-dfl.git
fi

ST=v5.15
cd stable
git checkout ${ST}
cd ..

BP=$PWD/linux-dfl-backport
M=${BP}/scripts/manifest.${ST}
INIT_GIT=0

if [ x$INIT_GIT = x1 ]; then
   if [ -d ${BP} ]; then
       rm -rf ${BP}
   fi
   mkdir ${BP}
   
   cd ${BP}
   git init .
else
    cd ${BP}
fi
mkdir scripts
cp ../scripts/*.sh scripts/
cp ../scripts/manifest.${ST} scripts/

git add scripts/*.sh scripts/manifest*
git commit -m "Import scripts"
    
git remote add pub ../stable.git
git remote add dfl ../linux-dfl.git
git remote update

while read -r line
do
    d=`dirname $line`
    mkdir -p $d
    if [ -f ../stable/$line ]; then
	cp ../stable/$line $line
    else
	mkdir -p $line
    fi
done < ${M}
git add *
git commit -m "Import from mainline ${ST}"

CF=v5.15.6
CT=ec2a46c7dd21ce150e64639067d27bb9fdfe8ea5
L=`git rev-list --no-merges --reverse ${CF}..${CT}`
for l in $L ; do
    git cherry-pick $l
    if [ $? != 0 ]; then
	git reset --hard
	git show $l
	exit 1
    fi
done

