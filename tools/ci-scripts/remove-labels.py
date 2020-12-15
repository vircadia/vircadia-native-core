import os, json
from github import Github

label_prefix = os.environ['LABEL_PREFIX']
context = json.loads(os.environ['GITHUB_CONTEXT'])
pr = Github(os.environ['GITHUB_TOKEN']).get_repo(context['repository']).get_pull(context['event']['number'])
for label in pr.get_labels():
    if label.name.startswith(label_prefix):
        pr.remove_from_labels(label.name)
