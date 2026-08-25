if [ -z "$SHELL_LOGGED_IN" ] && [ ! "$TERM_PROGRAM" = "vscode" ]; then
  cd "$WORKING_DIRECTORY"
  export SHELL_LOGGED_IN=true
fi

if [ ! -z "$SSH_CONNECTION" ]; then
  while read line
  do
      key=$(echo $line | sed "s/=.*//")
      value=$(echo $line | sed "s/$key=//1")
      decodedValue=$(echo $value | base64 -d)
      export $key="$decodedValue"
  done < /workspaces/.codespaces/shared/.env-secrets
fi
