#pragma once

#include <httpmicroservice/types.hpp>

namespace httpmicroservice {

/**
  Initialize a response with some sane values based on the request and a 200 OK
  status code.
 */
response make_response(const request& req);

/**
  Read the PORT environment variable. Returns 8080 if it is not set. Throws
  exception if the PORT variable is not an integer between 1024 and 65535.

  Cloud Run sets the PORT environment variable.
  https://cloud.google.com/run/docs/container-contract#port
 */
int getenv_port();

} // namespace httpmicroservice
