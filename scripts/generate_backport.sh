#!/bin/sh
set -x

cd ../..
if [ ! -d linux.git ]; then
    git clone --bare http://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
fi
if [ ! -d linux ]; then
   git clone linux.git
fi

if [ ! -d linux-dfl.git ]; then
    git clone --bare https://github.com/OPAE/linux-dfl.git
fi

ST=v5.8
cd linux
git checkout ${ST}
cd ..

BP=$PWD/linux-dfl-backport
M=${BP}/scripts/manifest.${ST}

cd ${BP}
#git add scripts/*.sh scripts/manifest*
#git commit -m "Import scripts"
    
#git remote add dfl ../linux-dfl.git
#git remote update
#cd ..

while read -r line
do
    d=`dirname $line`
    mkdir -p $d
    if [ -f ../linux/$line ]; then
	cp ../linux/$line $line
    else
	mkdir -p $line
    fi
	   
done < ${M}
git add *
git commit -m "Import from mainline ${ST}"

CF=bcf876870b95592b52519ed4aafcf9d95999bc9c
CT=OFS-EA-4
L=`git rev-list --reverse ${CF}..${CT}`
for l in $L ; do
    git cherry-pick $l
    if [ $? != 0 ]; then
	git reset --hard
	git show $l
	exit 1
    fi
done
git tag -d ${CT}
git tag ${CT}
