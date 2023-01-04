#
# Read the list of paths from an Open API spec and make a request to each
# endpoint. If the response is a 200 OK, the response body is not empty, and
# the Content-Type header is sane then call the request a success.
#
# Use canned TEST_DATA rather than examples and schemas from the spec to try
# and keep it simple.
#
import pytest
import requests
import yaml

TEST_DATA = {
    'get': {
        'headers': {'Accept-Type': 'application/json'},
    },
    'post': {
        'headers': {'Accept-Type': 'text/plain', 'Content-Type': 'text/plain'},
        'data': 'Hello World'
    }
}


def prepare_request(method, url):
    """Return a requests.PreparedRequest object with optional test inputs for
    the request headers and body. Use canned test values from TEST_DATA.
    """
    test_obj = TEST_DATA.get(method, {})

    headers = test_obj.get('headers')
    data = test_obj.get('data')

    return requests.Request(method, url, headers=headers, data=data).prepare()


def openapi_to_requests():
    """Parse an OpenAPI document in YAML format. Return iterable of test
    requests that cover all paths and methods in the specification. Annotate
    with canned TEST_DATA if available.
    """
    with open('openapi.yaml', 'r') as f:
        doc = yaml.safe_load(f)

    # Use the first server url
    base = doc.get('servers', [])[0].get('url')

    # Each path supports one or more methods
    # /target:
    #   get: ...
    #   post: ...
    #
    # ("get /target", "post /target", ...)
    return (
        prepare_request(method, f'{base}{target}')
        for target, path in doc.get('paths', {}).items()
        for method in path
    )


@pytest.mark.parametrize('req', openapi_to_requests())
def test_openapi(req):
    """Given a request object, call the API and verify the results. Use
    parametrize to create a test case for each item in the iterable requests.
    """
    session = requests.Session()
    resp = session.send(req)

    assert resp.status_code == 200
    assert len(resp.text) > 0
    if 'Accept-Type' in req.headers:
        assert (
            resp.headers['Content-Type'] == req.headers['Accept-Type'])


if __name__ == '__main__':
    pytest.main()
