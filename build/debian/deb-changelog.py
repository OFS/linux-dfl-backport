#!/usr/bin/env python
# Copyright(c) 2022, Intel Corporation

import argparse
import re
import subprocess
import sys
import time

GIT_REFS_PATTERN = r'(?:\s*tag:)(?:\s+(?P<tag>.*))'
GIT_REFS_RE = re.compile(GIT_REFS_PATTERN)

CHANGELOG_HDR_FMT = 'linux-dfl-backport-driver ({}) stable; urgency=medium\n\n'
CHANGELOG_FTR_FMT = ' -- The OPAE Dev Team <opae@lists.01.org>  {}\n'


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('version', help='the release version in M.m.r format')
    return parser.parse_args()


def run_process(cmd):
    return subprocess.check_output(
            cmd, stderr=open('/dev/null', 'w')).decode('UTF-8').strip()


def git_hash_for(commit_ish):
    return run_process(['git', '--no-pager', 'log',
                        '-1', '--format=%h', commit_ish])


def git_subject_for(commit_ish):
    return run_process(['git', '--no-pager', 'log',
                        '-1', '--format=%s', commit_ish])


def git_last_tag():
    return run_process(['git', 'describe', '--tags', '--abbrev=0'])


def git_tags_for(commit_ish):
    refs = run_process(['git', '--no-pager', 'log',
                        '-1', '--format=%d', commit_ish])
    refs = refs.split(',')
    tags = []
    for r in refs:
        m = GIT_REFS_RE.match(r)
        if m:
            tags.append(m.group('tag'))
    return tags


def changelog_date_time():
    return time.strftime('%a, %d %b %Y %H:%M:%S %z', time.localtime())


def git_changelog_for(commit_ish, version):
    s = CHANGELOG_HDR_FMT.format(version)
    s += '  * {} {}\n\n'.format(git_hash_for(commit_ish),
                                git_subject_for(commit_ish))
    s += CHANGELOG_FTR_FMT.format(changelog_date_time())
    return s


def git_commits_in_range(start, stop):
    h = git_hash_for(stop)
    while h != start:
        yield h
        h = git_hash_for('{}~'.format(h))


def git_changelog_for_range(start, stop, args):
    s = CHANGELOG_HDR_FMT.format(args.version)
    for h in git_commits_in_range(git_hash_for(start), git_hash_for(stop)):
        s += '  * {} {}\n'.format(h, git_subject_for(h))
    s += '\n'
    s += CHANGELOG_FTR_FMT.format(changelog_date_time())
    return s


def main():
    args = parse_args()

    tags_for_HEAD = git_tags_for('HEAD')
    if tags_for_HEAD:
        # If HEAD has tags, then output one changelog entry for HEAD.
        print(git_changelog_for('HEAD', args.version))
    else:
        try:
            last_tag = git_last_tag()
            print(git_changelog_for_range(last_tag, 'HEAD', args))
        except subprocess.CalledProcessError:
            print(git_changelog_for('HEAD', args.version))


if __name__ == '__main__':
    main()
