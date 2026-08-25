if status --is-login
  if [ -z "$SHELL_LOGGED_IN" ] && [ "$TERM_PROGRAM" != "vscode" ]
    cd $WORKING_DIRECTORY
    set -x SHELL_LOGGED_IN true
  end
end

if [ ! -z "$SSH_CONNECTION" ]
  for i in (cat /workspaces/.codespaces/shared/.env-secrets)
      set key (echo $i | sed "s/=.*//")
      set value (echo $i | sed "s/$key=//1")
      set decodedValue (echo $value | base64 -d | string collect)
      set -gx $key $decodedValue
  end
end
