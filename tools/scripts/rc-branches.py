#!/usr/bin/env python

import logging
import os
import re
import sys

import argparse

from git import Repo

FORMAT = '[%(levelname)s] %(message)s'
logging.basicConfig(format=FORMAT, level=logging.DEBUG)

remote_name = "upstream"
remote_master_branch = "{}/master".format(remote_name)


class VersionParser:
    """A parser for version numbers"""
    def __init__(self, versionString):
        # Validate that user passed a valid version
        VERSION_RE = re.compile(r"^v?(\d+)\.(\d+)\.(\d+)$")
        match = VERSION_RE.match(versionString)
        if not match:
            raise ValueError("Invalid version (should be X.Y.Z)")

        # Parse the version component and build proper version strings
        self.major = int(match.group(1))
        self.minor = int(match.group(2))
        self.patch = int(match.group(3))
        self.version = "v{}.{}.{}".format(self.major, self.minor, self.patch) # clean up version

        self.is_major_release = False
        self.is_minor_release = False
        self.is_patch_release = False
        if self.patch != 0:
            self.is_patch_release = True
            self.previous_version = "v{}.{}.{}".format(self.major, self.minor, self.patch - 1)
        elif self.minor != 0:
            self.is_minor_release = True
            self.previous_version = "v{}.{}.{}".format(self.major, self.minor - 1, 0)
        else:
            self.is_major_release = True
            self.previous_version = "v{}.{}.{}".format(self.major - 1, 0, 0)
            raise ValueError("Major releases not yet supported")

        # Build the branch names
        self.previous_rc_branch = "{}-rc".format(self.previous_version)
        self.base_branch = "{}-rc-base".format(self.version)
        self.rc_branch = "{}-rc".format(self.version)
        self.remote_previous_rc_branch = "{}/{}".format(remote_name, self.previous_rc_branch)
        self.remote_base_branch = "{}/{}".format(remote_name, self.base_branch)
        self.remote_rc_branch = "{}/{}".format(remote_name, self.rc_branch)


def checkVersionBranches(version):
    """Check that the branches for a given version were created properly."""

    parser = VersionParser(version)

    repo = Repo(os.getcwd(), search_parent_directories=True)
    assert not repo.bare

    # Verify the branches' existance
    if parser.remote_previous_rc_branch not in repo.refs:
        raise ValueError("Previous RC branch not found: {}".format(parser.remote_previous_rc_branch))

    if parser.remote_base_branch not in repo.refs:
        raise ValueError("Base branch not found: {}".format(parser.remote_base_branch))

    if parser.remote_rc_branch not in repo.refs:
        raise ValueError("RC branch not found: {}".format(parser.remote_rc_branch))

    previous_rc = repo.refs[parser.remote_previous_rc_branch]
    current_rc_base = repo.refs[parser.remote_base_branch]
    current_rc = repo.refs[parser.remote_rc_branch]
    master = repo.refs[remote_master_branch]

    # Check the base branch is an ancestor of the rc branch
    if not repo.is_ancestor(current_rc_base, current_rc):
        raise ValueError("{} is not an ancesctor of {}".format(current_rc_base, current_rc))

    # Check that the base branch is the merge base of the previous and current RCs
    merge_base = repo.merge_base(previous_rc, current_rc)
    if current_rc_base.commit not in merge_base:
        raise ValueError("Base branch is not the merge base between {} and {}".format(previous_rc, current_rc))

    # For patch releases, warn if the base commit is not the previous RC commit
    if parser.is_patch_release:
        if parser.previous_version not in repo.tags:
            logging.warning("The tag {0} does not exist, which suggests {0} has not been released.".format(parser.previous_version))

        if current_rc_base.commit != previous_rc.commit:
            logging.warning("Previous version has commits not in this patch");
            logging.warning("Type \"git diff {}..{}\" to see the commit list".format(current_rc_base, previous_rc));

        # Check base branch is part of the previous RC
        previous_rc_base_commit = repo.merge_base(previous_rc, master)
        if repo.is_ancestor(current_rc_base, previous_rc_base_commit):
            raise ValueError("{} is older than {}".format(current_rc_base, previous_rc))

    print("[SUCCESS] Checked {}".format(parser.version))

def createVersionBranches(version):
    """Create the branches for a given version."""

    parser = VersionParser(version)

    repo = Repo(os.getcwd(), search_parent_directories=True)
    assert not repo.bare

    # Validate the user is on a local branch that has the right merge base
    if repo.head.is_detached:
        raise ValueError("You must not run this script in a detached HEAD state")

    # Validate the user has no pending changes
    if repo.is_dirty():
        raise ValueError("Your working tree has pending changes. You must have a clean working tree before proceeding.")

    # Make sure the remote is up to date
    remote = repo.remotes[remote_name]
    remote.fetch(prune=True)

    # Verify the previous RC branch exists
    if parser.remote_previous_rc_branch not in repo.refs:
        raise ValueError("Previous RC branch not found: {}".format(parser.remote_previous_rc_branch))

    # Verify the branches don't already exist
    if parser.remote_base_branch in repo.refs:
        raise ValueError("Base branch already exists: {}".format(parser.remote_base_branch))

    if parser.remote_rc_branch in repo.refs:
        raise ValueError("RC branch already exists: {}".format(parser.remote_rc_branch))

    if parser.base_branch in repo.refs:
        raise ValueError("Base branch already exists locally: {}".format(parser.base_branch))

    if parser.rc_branch in repo.refs:
        raise ValueError("RC branch already exists locally: {}".format(parser.rc_branch))

    # Save current branch name
    current_branch_name = repo.active_branch

    # Create the RC branches
    if parser.is_patch_release:
        # Check tag exists, if it doesn't, print warning and ask for comfirmation
        if parser.previous_version not in repo.tags:
            logging.warning("The tag {0} does not exist, which suggests {0} has not yet been released.".format(parser.previous_version))
            logging.warning("Creating the branches now means that {0} will diverge from {1} if anything is merged into {1}.".format(parser.version, parser.previous_version))
            logging.warning("This is not recommended unless necessary.")

            validAnswer = False
            askCount = 0
            while not validAnswer and askCount < 3:
                answer = input("Are you sure you want to do this? [y/n] ").strip().lower()
                askCount += 1
                validAnswer = answer == "y" or answer == "n"

            if not validAnswer:
                raise ValueError("Did not understand response")

            if answer == "n":
                print("Aborting")
                return
            else:
                print("Creating branches")

        previous_rc = repo.refs[parser.remote_previous_rc_branch]

        repo.create_head(parser.base_branch, previous_rc)
        remote.push("{0}:{0}".format(parser.base_branch))
        repo.create_head(parser.rc_branch, previous_rc)
        remote.push("{0}:{0}".format(parser.rc_branch))
    else:
        previous_rc = repo.refs[parser.remote_previous_rc_branch]
        master = repo.refs[remote_master_branch]
        merge_base = repo.merge_base(previous_rc, master)

        repo.create_head(parser.base_branch, merge_base[0])
        remote.push("{0}:{0}".format(parser.base_branch))
        repo.create_head(parser.rc_branch, master)
        remote.push("{0}:{0}".format(parser.rc_branch))

    print("[SUCCESS] Created {} and {}".format(parser.base_branch, parser.rc_branch))
    print("[SUCCESS] You can make the PR from the following webpage:")
    print("[SUCCESS] https://github.com/highfidelity/hifi/compare/{}...{}".format(parser.base_branch, parser.rc_branch))
    if parser.is_patch_release:
        print("[SUCCESS] NOTE: You will have to wait for the first fix to be merged into the RC branch to be able to create the PR")

def main():
    """Execute Main entry point."""
    global remote_name

    parser = argparse.ArgumentParser(description='RC branches tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''Example commands you can run:\n%(prog)s check 0.75.0\n%(prog)s create 0.75.1''')
    parser.add_argument("command", help="command to execute", choices=["check", "create"])
    parser.add_argument("version", help="version of the form \"X.Y.Z\"")
    parser.add_argument("--remote", dest="remote_name", default=remote_name,
        help="git remote to use as reference")
    args = parser.parse_args()

    remote_name = args.remote_name

    try:
        if args.command == "check":
            checkVersionBranches(args.version)
        elif args.command == "create":
            createVersionBranches(args.version)
        else:
            parser.print_help()
    except ValueError as ex:
        logging.error(ex)
        sys.exit(1)

if __name__ == "__main__":
    main()
