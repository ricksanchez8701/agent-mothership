if [ ! -z "$SSH_CONNECTION" ]; then
  foreach  i ("`cat /workspaces/.codespaces/shared/.env-secrets`")
      set key = `echo $i | sed "s/=.*//"`
      set value = `echo $i | sed "s/$key=//1"`
      setenv $key `echo $value | base64 -d`
  end
fi

if (! $?SHELL_LOGGED_IN) then
  if ($?TERM_PROGRAM) then
    if ($TERM_PROGRAM != "vscode") then
        cd "$WORKING_DIRECTORY"
    endif
  else
    cd "$WORKING_DIRECTORY"
  endif
  setenv SHELL_LOGGED_IN true
endif