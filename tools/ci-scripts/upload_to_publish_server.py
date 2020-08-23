import os
import json
from hashlib import sha256
import http.client
from http import HTTPStatus
import time
import struct
import random
import glob

FILE_READ_BUFFER = 4096

path = os.path.join(os.getcwd(), os.environ['ARTIFACT_PATTERN'])
files = glob.glob(path, recursive=False)
uploading_files = []
for archive_file in files:
    file = open(archive_file, 'rb')
    sha256_hash = sha256()
    file.seek(0, 0)
    for byte_block in iter(lambda: file.read(FILE_READ_BUFFER), b""):
        sha256_hash.update(byte_block)

    checksum = sha256_hash.hexdigest()

    uploading_files.append({
        "filename": os.path.basename(archive_file),
        "sha256_checksum": checksum,
        "file_length": file.tell()
    })
    file.close()

print("BuildFileHashes: " + json.dumps(uploading_files))

file_contents = []
file_sizes = []

for archiveFile in files:
    file = open(archiveFile, 'rb')
    file_data = file.read()
    file_sizes.append(len(file_data))
    file_contents.append(file_data)
    file.close()

conn = http.client.HTTPSConnection("build-uploader.vircadia.com")

context = json.loads(os.environ['GITHUB_CONTEXT'])

owner_and_repository = context["repository"].split("/")
owner = owner_and_repository[0]
repository = owner_and_repository[1]

headers = {
    "owner": owner,
    "repo": repository,
    "commit_hash": context["event"]["pull_request"]["head"]["sha"],
    "pull_number": context["event"]["number"],
    "job_name": os.environ["JOB_NAME"],
    "run_id": context["run_id"],
    "file_sizes": ','.join(str(e) for e in file_sizes)
}

concat_file_body = b''.join(file_contents)

print("Total files size: " + str(len(concat_file_body)))

conn.request("PUT", "/", body=concat_file_body, headers=headers)
response = conn.getresponse()

EXIT_CODE_OK = 0
EXIT_CODE_ERROR = 1

if (response.status == HTTPStatus.OK):
    print("response: ",  json.loads(response.read()))
    exit(EXIT_CODE_OK)
else:
    print(response.status, response.reason, response.read())
    exit(EXIT_CODE_ERROR)
