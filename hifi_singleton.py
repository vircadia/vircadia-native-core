import json
import logging
import os
import platform
import time

try:
    import fcntl
except ImportError:
    fcntl = None

try:
    import msvcrt
except ImportError:
    msvcrt = None


logger = logging.getLogger(__name__)


# Used to ensure only one instance of the script runs at a time
class Singleton:
    def __init__(self, path):
        self.fh = None
        self.windows = 'Windows' == platform.system()
        self.path = path

    def __enter__(self):
        success = False
        while not success:
            try:
                if self.windows:
                    if os.path.exists(self.path):
                        os.unlink(self.path)
                    self.fh = os.open(self.path, os.O_CREAT | os.O_EXCL | os.O_RDWR)
                else:
                    self.fh = open(self.path, 'x')
                    fcntl.lockf(self.fh, fcntl.LOCK_EX | fcntl.LOCK_NB)
                success = True
            except EnvironmentError as err:
                if self.fh is not None:
                    if self.windows:
                        os.close(self.fh)
                    else:
                        self.fh.close()
                    self.fh = None
                # print is horked here so write directly to stdout.
                with open(1, mode="w", closefd=False) as _stdout:
                    _stdout.write("Couldn't aquire lock, retrying in 10 seconds\n")
                    _stdout.flush()
                time.sleep(10)
        return self

    def __exit__(self, type, value, traceback):
        if self.windows:
            os.close(self.fh)
        else:
            fcntl.lockf(self.fh, fcntl.LOCK_UN)
            self.fh.close()
        os.unlink(self.path)


class FLock:
    """
    File locking context manager

    >> with FLock("/tmp/foo.lock"):
    >>     do_something_that_must_be_synced()

    The lock file must stick around forever. The author is not aware of a no cross platform way to clean it up w/o introducting race conditions.
    """
    def __init__(self, path):
        self.fh = os.open(path, os.O_CREAT | os.O_RDWR)
        self.path = path

    def _lock_posix(self):
        try:
            fcntl.lockf(self.fh, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            # Windows sleeps for 10 seconds before giving up on a lock.
            # Lets mimic that behavior.
            time.sleep(10)
            return False
        else:
            return True

    def _lock_windows(self):
        try:
            msvcrt.locking(self.fh, msvcrt.LK_LOCK, 1)
        except OSError:
            return False
        else:
            return True

    if fcntl is not None:
        _lock = _lock_posix
    elif msvcrt is not None:
        _lock = _lock_windows
    else:
        raise RuntimeError("No locking library found")

    def read_stats(self):
        data = {}
        with open(self.fh, mode="r", closefd=False) as stats_file:
            stats_file.seek(0)
            try:
                data = json.loads(stats_file.read())
            except json.decoder.JSONDecodeError:
                logger.warning("couldn't decode json in lock file")
            except PermissionError:
                # Can't read a locked file on Windows :(
                pass

        lock_age = time.time() - os.fstat(self.fh).st_mtime
        if lock_age > 0:
            data["Age"] = "%0.2f" % lock_age

        with open(1, mode="w", closefd=False) as _stdout:
            _stdout.write("Lock stats:\n")
            for key, value in sorted(data.items()):
                _stdout.write("* %s: %s\n" % (key, value))
            _stdout.flush()

    def write_stats(self):
        stats = {
            "Owner PID": os.getpid(),
        }
        flock_env_vars = os.getenv("FLOCK_ENV_VARS")
        if flock_env_vars:
            for env_var_name in flock_env_vars.split(":"):
                stats[env_var_name] = os.getenv(env_var_name)

        with open(self.fh, mode="w", closefd=False) as stats_file:
            stats_file.truncate()
            return stats_file.write(json.dumps(stats, indent=2))

    def __enter__(self):
        while not self._lock():
            try:
                self.read_stats()
            except (IOError, ValueError) as exc:
                logger.exception("couldn't read stats")
                time.sleep(3.33)  # don't hammer the file

        self.write_stats()

        return self

    def __exit__(self, type, value, traceback):
        os.close(self.fh)
        # WARNING: `os.close` gives up the lock on `fh` then we attempt the `os.unlink`. On posix platforms this can lead to us deleting a lock file that another process owns. This step is required to maintain compatablity with Singleton. When and if FLock is completely rolled out to the build fleet this unlink should be removed.
        try:
            os.unlink(self.path)
        except (FileNotFoundError, PermissionError):
            logger.exception("couldn't unlink lock file")


if os.getenv("USE_FLOCK_CLS") is not None:
    logger.warning("Using FLock locker")
    Singleton = FLock
