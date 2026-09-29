import os
import subprocess
import sys
import time

# The build folder is restored from the cache of a previous run, while a fresh
# checkout gives every source file the current time, which would force a full
# rebuild. Mark all tracked files as old and only the files changed since the
# previously built commit as new, so the build tool recompiles just those.

OLD_TIME = 946684800

repo = sys.argv[1]
sha_file = os.path.join(repo, 'out', 'ayu_build_sha.txt')


def git(*args):
    return subprocess.run(
        ['git', '-C', repo, *args],
        capture_output=True,
        text=True,
        encoding='utf-8')


def changed_since(previous):
    if git('cat-file', '-e', previous + '^{commit}').returncode != 0:
        git('fetch', '--depth=1', 'origin', previous)
    diff = git('diff', '--name-only', '-z', previous, 'HEAD')
    if diff.returncode != 0:
        return None
    return {path for path in diff.stdout.split('\0') if path}


previous = ''
if os.path.exists(sha_file):
    with open(sha_file, encoding='utf-8') as f:
        previous = f.read().strip()
changed = changed_since(previous) if previous else None
if changed is None:
    print('No usable previous build, doing a full build.')
    sys.exit(0)

submodules = git(
    'config', '--file', '.gitmodules', '--get-regexp', 'path'
).stdout.split()[1::2]
changed_submodules = [path + '/' for path in submodules if path in changed]

now = time.time()
touched = 0
files = git('ls-files', '--recurse-submodules', '-z').stdout.split('\0')
for relative in files:
    if not relative:
        continue
    path = os.path.join(repo, relative)
    if not os.path.isfile(path):
        continue
    is_new = (relative in changed) or any(
        relative.startswith(prefix) for prefix in changed_submodules)
    stamp = now if is_new else OLD_TIME
    os.utime(path, (stamp, stamp))
    touched += int(is_new)
print(f'Incremental build from {previous}: {touched} changed files.')
