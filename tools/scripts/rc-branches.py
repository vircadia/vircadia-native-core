#!/usr/bin/env python

import logging
import os
import re
import sys

import argparse

from utils import git

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
    major = parser.major
    minor = parser.minor
    patch = parser.patch
    previous_version = parser.previous_version
    version = parser.version

    is_major_release = parser.is_major_release
    is_minor_release = parser.is_minor_release
    is_patch_release = parser.is_patch_release

    remote_previous_rc_branch = parser.remote_previous_rc_branch
    remote_base_branch = parser.remote_base_branch
    remote_rc_branch = parser.remote_rc_branch

    repo = git.Repository(git.Repository.get_base_directory())

    # Verify the branches' existance
    if not repo.does_branch_exist(remote_previous_rc_branch):
        raise ValueError("Previous RC branch not found: {}".format(remote_previous_rc_branch))

    if not repo.does_branch_exist(remote_base_branch):
        raise ValueError("Base branch not found: {}".format(remote_base_branch))

    if not repo.does_branch_exist(remote_rc_branch):
        raise ValueError("RC branch not found: {}".format(remote_rc_branch))

    # Figure out SHA for each of the branches
    previous_rc_commit = repo.git_rev_parse([remote_previous_rc_branch])
    base_commit = repo.git_rev_parse([remote_base_branch])
    rc_commit = repo.git_rev_parse([remote_rc_branch])

    # Check the base branch is an ancestor of the rc branch
    if not repo.is_ancestor(base_commit, rc_commit):
        raise ValueError("{} is not an ancesctor of {}".format(remote_base_branch, remote_rc_branch))

    # Check that the base branch is the merge base of the previous and current RCs
    merge_base = repo.get_merge_base(previous_rc_commit, rc_commit)
    if base_commit != merge_base:
        raise ValueError("Base branch is not the merge base between {} and {}".format(remote_previous_rc_branch, remote_rc_branch))

    # For patch releases, warn if the base commit is not the previous RC commit
    if is_patch_release:
        if not repo.does_tag_exist(version):
            logging.warning("The tag {} does not exist, which suggests {} has not been released.".format(version, version))

        if base_commit != previous_rc_commit:
            logging.warning("Previous version has commits not in this patch");
            logging.warning("Type \"git diff {}..{}\" to see the commit list".format(base_commit, previous_rc_commit));

        # Check base branch is part of the previous RC
        previous_rc_base_commit = repo.get_merge_base(previous_rc_commit, remote_master_branch)
        if repo.is_ancestor(base_commit, previous_rc_base_commit):
            raise ValueError("{} is older than {}".format(remote_base_branch, remote_rc_branch))

    print("[SUCCESS] Checked {}".format(version))

def createVersionBranches(version):
    """Create the branches for a given version."""

    parser = VersionParser(version)
    major = parser.major
    minor = parser.minor
    patch = parser.patch
    previous_version = parser.previous_version
    version = parser.version

    is_major_release = parser.is_major_release
    is_minor_release = parser.is_minor_release
    is_patch_release = parser.is_patch_release

    previous_rc_branch = parser.previous_rc_branch
    base_branch = parser.base_branch
    rc_branch = parser.rc_branch
    remote_previous_rc_branch = parser.remote_previous_rc_branch
    remote_base_branch = parser.remote_base_branch
    remote_rc_branch = parser.remote_rc_branch

    repo = git.Repository(git.Repository.get_base_directory())

    # Validate the user is on a local branch that has the right merge base
    if repo.is_detached():
        raise ValueError("You must not run this script in a detached HEAD state")

    # Validate the user has no pending changes
    if repo.is_working_tree_dirty():
        raise ValueError("Your working tree has pending changes. You must have a clean working tree before proceeding.")

    # Make sure the remote is up to date
    repo.git_fetch([remote_name])

    # Verify the previous RC branch exists
    if not repo.does_branch_exist(remote_previous_rc_branch):
        raise ValueError("Previous RC branch not found: {}".format(remote_previous_rc_branch))

    # Verify the branches don't already exist
    if repo.does_branch_exist(remote_base_branch):
        raise ValueError("Base branch already exists: {}".format(remote_base_branch))

    if repo.does_branch_exist(remote_rc_branch):
        raise ValueError("RC branch already exists: {}".format(remote_rc_branch))

    if repo.does_branch_exist(base_branch):
        raise ValueError("Base branch already exists locally: {}".format(base_branch))

    if repo.does_branch_exist(rc_branch):
        raise ValueError("RC branch already exists locally: {}".format(rc_branch))

    # Save current branch name
    current_branch_name = repo.get_branch_name()

    # Create the RC branches
    if is_patch_release:

        # Check tag exists, if it doesn't, print warning and ask for comfirmation
        if not repo.does_tag_exist(previous_version):
            logging.warning("The tag {} does not exist, which suggests {} has not yet been released.".format(previous_version, previous_version))
            logging.warning("Creating the branches now means that {} will diverge from {} if anything is merged into {}.".format(version, previous_version, previous_version))
            logging.warning("This is not recommended unless necessary.")

            validAnswer = False
            askCount = 0
            while not validAnswer and askCount < 3:
                answer = input("Are you sure you want to do this? [y/n]").strip().lower()
                askCount += 1
                validAnswer = answer == "y" or answer == "n"

            if not validAnswer:
                raise ValueError("Did not understand response")

            if answer == "n":
                print("Aborting")
                return
            else:
                print("Creating branches")

        repo.git_checkout(["-b", base_branch, remote_previous_rc_branch])
        repo.push_to_remote_branch(remote_name, base_branch)
        repo.git_checkout(["-b", rc_branch, remote_previous_rc_branch])
        repo.push_to_remote_branch(remote_name, rc_branch)
    else:
        merge_base = repo.get_merge_base(remote_previous_rc_branch, remote_master_branch)
        repo.git_checkout(["-b", base_branch, merge_base])
        repo.push_to_remote_branch(remote_name, base_branch)
        repo.git_checkout(["-b", rc_branch, remote_master_branch])
        repo.push_to_remote_branch(remote_name, rc_branch)

    repo.git_checkout([current_branch_name])

    print("[SUCCESS] Created {} and {}".format(base_branch, rc_branch))
    print("[SUCCESS] You can make the PR from the following webpage:")
    print("[SUCCESS] https://github.com/highfidelity/hifi/compare/{}...{}".format(base_branch, rc_branch))
    if is_patch_release:
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
    except Exception as ex:
        logging.error(ex)
        sys.exit(1)

if __name__ == "__main__":
    main()
