Record the forks, clones and execs of a process and display as a tree.

Example:

```console
$ ./process-tree git vibranch
Deleted branch test-branch (was 5348c61).
git vibranch
└───bash /home/ludviggl/.nix-profile/bin/git-vibranch
    ├───159117
    │   ├───git branch
    │   └───sed s/\*//g
    ├───mktemp
    ├───hx /tmp/tmp.o64lnhu4iV
    │   ├───159122
    │   ├───159123
    │   ├───159124
    │   ├───159125
    │   ├───159126
    │   ├───159127
    │   ├───159128
    │   ├───159129
    │   ├───159130
    │   └───159131
    ├───git branch -D test-branch
    └───rm /tmp/tmp.o64lnhu4iV
```
