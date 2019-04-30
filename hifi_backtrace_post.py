# Parameters:
#   1 - $BACKTRACE_UPLOAD_TOKEN
#   2 - $SYMBOLS_ARCHIVE
#   3 - $RELEASE_NUMBER
#
import sys
import urllib.request
import urllib.parse

print("Running python script to upload to BackTrace")

post_headers = {}
post_headers['Content-Type'] = 'application/json'
post_headers['Expect'] = ''

post_url = 'https://highfidelity.sp.backtrace.io:6098/post?format=symbols&token=' + sys.argv[1] +  '&upload_file=' + sys.argv[2] + '&tag=' + sys.argv[3]

try:
    post_data = open(sys.argv[2], 'rb')
except:
    print('file ' + sys.argv[2] + ' not found')
    exit()
    
try:
    post_request = urllib.request.Request(post_url, post_data, post_headers)
except:
    print('urllib.request.Request failed')
    exit()

try:
    post_response = urllib.request.urlopen(post_request)
except:
    print('urllib.request.urlopen failed')
    exit()

print("Upload to BackTrace completed without errors")