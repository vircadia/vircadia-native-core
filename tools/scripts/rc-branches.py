#!/usr/bin/env python

import logging
import os
import re
import sys

from optparse import OptionParser

from utils import git

FORMAT = '[%(levelname)s] %(message)s'
logging.basicConfig(format=FORMAT, level=logging.DEBUG)

remote_name = "upstream"
remote_master_branch = "{}/master".format(remote_name)

def checkVersionBranches(version):
    """Check the branches for a given version were created properly."""

    repo = git.Repository(git.Repository.get_base_directory())

    # Validate that user passed a valid version
    VERSION_RE = re.compile(r"^v?(\d+)\.(\d+)\.(\d+)$")
    match = VERSION_RE.match(version)
    if not match:
        raise ValueError("Invalid version (should be X.Y.Z)")

    # Parse the version component and build proper version strings
    major = int(match.group(1))
    minor = int(match.group(2))
    patch = int(match.group(3))
    version = "v{}.{}.{}".format(major, minor, patch) # clean up version

    is_major_release = False
    is_minor_release = False
    is_patch_release = False
    if patch != 0:
        is_patch_release = True
        previous_version = "v{}.{}.{}".format(major, minor, patch - 1)
    elif minor != 0:
        is_minor_release = True
        previous_version = "v{}.{}.{}".format(major, minor - 1, 0)
    else:
        is_major_release = True
        raise ValueError("Major releases not yet supported")

    # Build the branch names
    previous_rc_branch = "{}/{}-rc".format(remote_name, previous_version)
    base_branch = "{}/{}-rc-base".format(remote_name, version)
    rc_branch = "{}/{}-rc".format(remote_name, version)

    # Verify the branches' existance
    if not repo.does_branch_exist(previous_rc_branch):
        raise ValueError("Previous RC branch not found: {}".format(previous_rc_branch))

    if not repo.does_branch_exist(base_branch):
        raise ValueError("Base branch not found: {}".format(base_branch))

    if not repo.does_branch_exist(rc_branch):
        raise ValueError("RC branch not found: {}".format(rc_branch))

    # Figure out SHA for each of the branches
    previous_rc_commit = repo.git_rev_parse([previous_rc_branch])
    base_commit = repo.git_rev_parse([base_branch])
    rc_commit = repo.git_rev_parse([rc_branch])

    # Check the base branch is an ancestor of the rc branch
    if not repo.is_ancestor(base_commit, rc_commit):
        raise ValueError("{} is not an ancesctor of {}".format(base_branch, rc_branch))

    # Check that the base branch is the merge base of the previous and current RCs
    merge_base = repo.get_merge_base(previous_rc_commit, rc_commit)
    if base_commit != merge_base:
        raise ValueError("Base branch is not the merge base between {} and {}".format(previous_rc_branch, rc_branch))

    # For patch releases, warn if the base commit is not the previous RC commit
    if is_patch_release:
        if not repo.does_tag_exist(version):
            logging.warning("The tag {} does not exist, which suggests {} hasn't yet been cut.".format(version, version))

        if base_commit != previous_rc_commit:
            logging.warning("Previous version has commits not in this patch");
            logging.warning("Type \"git diff {}..{}\" to see the commit list".format(base_commit, previous_rc_commit));

        # Check base branch is part of the previous RC
        previous_rc_base_commit = repo.get_merge_base(previous_rc_commit, remote_master_branch)
        if repo.is_ancestor(base_commit, previous_rc_base_commit):
            raise ValueError("{} is older than {}".format(base_branch, rc_branch))

    print("[SUCCESS] Checked {}".format(version))

def createVersionBranches(version):
    """Create the branches for a given version."""

    repo = git.Repository(git.Repository.get_base_directory())

    # Validate the user is on a local branch that has the right merge base
    if repo.is_detached():
        raise ValueError("You must not run this script in a detached HEAD state")

    # Validate the user has no pending changes
    if repo.is_working_tree_dirty():
        raise ValueError("Your working tree has pending changes. You must have a clean working tree before proceeding.")

    # Validate that user passed a valid version
    VERSION_RE = re.compile(r"^v?(\d+)\.(\d+)\.(\d+)$")
    match = VERSION_RE.match(version)
    if not match:
        raise ValueError("Invalid version (should be X.Y.Z)")

    # Parse the version component and build proper version strings
    major = int(match.group(1))
    minor = int(match.group(2))
    patch = int(match.group(3))
    version = "v{}.{}.{}".format(major, minor, patch) # clean up version

    is_major_release = False
    is_minor_release = False
    is_patch_release = False
    if patch != 0:
        is_patch_release = True
        previous_version = "v{}.{}.{}".format(major, minor, patch - 1)
    elif minor != 0:
        is_minor_release = True
        previous_version = "v{}.{}.{}".format(major, minor - 1, 0)
    else:
        is_major_release = True
        raise ValueError("Major releases not yet supported")

    # Build the branch names
    previous_rc_branch = "{}-rc".format(previous_version)
    base_branch = "{}-rc-base".format(version)
    rc_branch = "{}-rc".format(version)
    remote_previous_rc_branch = "{}/{}".format(remote_name, previous_rc_branch)
    remote_base_branch = "{}/{}".format(remote_name, base_branch)
    remote_rc_branch = "{}/{}".format(remote_name, rc_branch)

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
        if not repo.does_tag_exist(version):
            logging.warning("The tag {} does not exist, which suggests {} hasn't yet been cut.".format(version, version))
            logging.warning("Creating the branches now means that they will diverge from {} if anything is merge into it.".format(previous_version))
            logging.warning("This is not recommanded unless necessary.")

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

def usage():
    """Print usage."""
    print("rc-branches.py supports the following commands:")
    print("\ncheck <version>")
    print("   <version>  - version to check of the form \"X.Y.Z\"")
    print("\ncreate <version>")
    print("   <version>  - version to create branches for of the form \"X.Y.Z\"")

def main():
    """Execute Main entry point."""
    global remote_name

    parser = OptionParser()
    parser.add_option("-r", "--remote", type="string", dest="remote_name", default=remote_name)
    (options, args) = parser.parse_args(args=sys.argv)

    remote_name = options.remote_name

    if len(args) < 3:
        usage()
        return

    command = args[1]
    version = args[2]

    try:
        if command == "check":
            checkVersionBranches(version)
        elif command == "create":
            createVersionBranches(version)
        else:
            usage()
    except Exception as ex:
        logging.error(ex)
        sys.exit(1)

if __name__ == "__main__":
    main()
