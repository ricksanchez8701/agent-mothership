#!/bin/sh

# See https://git-scm.com/docs/git-credential for credential helper spec details
# v2

# This helper is somewhat strict as it's intended to work exactly in the codespaces scenario.
# Note: the script should always exit 0, even it it doesn't handle the request. Otherwise the git command will fail.

# This script may also be invoked with "store" or "erase" (or future commands), but we only need to handle "get"
if [ "$1" != "get" ]; then
    exit 0
fi

# If we don't have a token, there's nothing to do
if [ "$GITHUB_TOKEN" = "" ]; then
    exit 0
fi

# We need to match the server URL - if we don't know it, there's nothing to do
if [ "$GITHUB_SERVER_URL" = "" ]; then
    exit 0
fi

url=
protocol=
host=

# Parse options line by line on stdin
while IFS="\n" read -r line; do
    # Parse the line as key=value Note, the heredoc syntax here is weird, but
    # neccessary as sh doesn't have good support for using read on a variable.
    IFS="=" read -r key value rest << EOF
$line
EOF

    # If there are more than 2 fields, skip. Note there are valid config options
    # that have more than 2 fields, but we don't care about them.
    if [ "$rest" != "" ]; then
        continue
    fi

    # Extract the options we care about
    if [ "$key" = "protocol" ]; then
        protocol=$value
    elif [ "$key" = "host" ]; then
        host=$value
    elif [ "$key" = "url" ]; then
        url=$value
    fi
done

if [ "$url" = "" ]; then
    if [ "$protocol" != "" -a "$host" != "" ]; then
        url="$protocol://$host"
    else
        exit 0
    fi
fi

# Finally, if the url matches, output the credentials
if [ "$url" = "$GITHUB_SERVER_URL" ]; then
    echo username=PersonalAccessToken
    echo password=$GITHUB_TOKEN
fi
