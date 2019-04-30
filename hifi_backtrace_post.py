# Parameters:
#   1 - $BACKTRACE_UPLOAD_TOKEN
#   2 - $SYMBOLS_ARCHIVE
#   3 - $RELEASE_NUMBER
#
import sys
import urllib.request
import urllib.parse

post_headers = {}
post_headers['Content-Type'] = 'application/json'
post_headers['Expect'] = ''

post_url = 'https://highfidelity.sp.backtrace.io:6098/post?format=symbols&token=' + sys.argv[1] +  '&upload_file=' + sys.argv[2] + '&tag=' + sys.argv[3]
post_data = open(sys.argv[1], 'rb')

post_request = urllib.request.Request(post_url, post_data, post_headers)
post_response = urllib.request.urlopen(post_request)
