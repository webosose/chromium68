#!/usr/bin/env python

import json
import shelve
import os
import sys
import mimetypes
import socket

from wsgiref.util import setup_testing_defaults
from wsgiref.simple_server import make_server


def ok(start_response, body, ct='application/json; charset=UTF-8', ce=None):
    headers = [('Content-Type', ct)]
    if ce:
        headers.append(('Content-Encoding', ce))

    start_response('200 Ok', headers)
    return [body]


def bad(start_response, body, ct='application/json; charset=UTF-8', ce=None):
    headers = [('Content-Type', ct)]
    if ce:
        headers.append(('Content-Encoding', ce))

    start_response('400 Bad request', headers)
    return [body]


def content_type_magic(filename):
    mime, encoding = mimetypes.guess_type(filename)

    if not mime:
        mime = 'text/plain'

    return (mime, encoding)


def app(environ, start_response):
    setup_testing_defaults(environ)

    request_method = environ['REQUEST_METHOD']
    path_info = environ['PATH_INFO']
    resource = os.path.join(root, path_info.lstrip('/'))

    if request_method == "GET" and path_info == "/__IP__":
       s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
       s.connect(('a.root-servers.net.', 0))
       server_ip, _ = s.getsockname()
       return ok(start_response, server_ip, 'text/plain')

    if request_method == "GET" and os.path.exists(resource):
        if os.path.isfile(resource):
            return ok(start_response,
                      open(resource).read(),
                      *content_type_magic(resource))

        if os.path.isdir(resource):
            return ok(start_response,
                      '\n'.join(
                        '<p><a href="%(fn)s">%(fn)s</a></p>' % {'fn': fn}
                        for fn in os.listdir(resource)
                      ),
                      'text/html')

    if request_method != "POST":
        return bad(start_response, '{"result": "Invalid request"}')

    content_type = environ['CONTENT_TYPE']
    if not content_type.startswith("application/json"):
        return bad(start_response, '{"result": "Invalid Content-Type"}')

    try:
        content_length = int(environ['CONTENT_LENGTH'])
        body = environ['wsgi.input'].read(content_length)
    except Exception as e:
        return [e.message]

    try:
        data = json.loads(body)
        name = data['name'].encode('utf-8')
    except ValueError:
        return bad(start_response,
                   '{"result": "Invalid JSON"}')
    except KeyError:
        return bad(start_response,
                   '''"{result": "Required 'name' property is absent"}''')

    storage[name] = data
    storage.sync()

    return ok(start_response,
              (
               '''{"result": "Stored successfully at name='%s'"}''' % name
              ).encode('utf-8'))


try:
    port = int(sys.argv[1])
    root = sys.argv[2]
except (IndexError, ValueError):
    print >> sys.stderr, "Usage: %s <port> <root-path>" % sys.argv[0]
    exit(1)

storage = shelve.open('results.shelve')
httpd = make_server('', port, app)
print "Serving on port %d..." % port

try:
    httpd.serve_forever()
except KeyboardInterrupt:
    storage.close()
