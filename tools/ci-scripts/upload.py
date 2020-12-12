# Post build script
import os
import sys
import json
import boto3
import glob
from github import Github

def main():
    bucket_name = os.environ['UPLOAD_BUCKET']
    upload_prefix = os.environ['UPLOAD_PREFIX']
    release_number = os.environ['RELEASE_NUMBER']
    full_prefix = upload_prefix + '/' + release_number
    S3 = boto3.client('s3')
    path = os.path.join(os.getcwd(), os.environ['ARTIFACT_PATTERN'])
    files = glob.glob(path, recursive=False)
    for archiveFile in files:
        filePath, fileName = os.path.split(archiveFile)
        S3.upload_file(os.path.join(filePath, fileName), bucket_name, full_prefix + '/' + fileName)
        print("Uploaded Artifact to S3: https://{}.s3-eu-west-3.amazonaws.com/{}/{}".format(bucket_name, full_prefix, fileName))
    print("Finished")

main()
