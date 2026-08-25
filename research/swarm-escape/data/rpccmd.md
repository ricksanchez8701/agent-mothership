# Agent rpccmd — every command the agent executes on OUR input

## RPC -> program -> where -> user input reach
| RPC | program | where | user input |
|---|---|---|---|
| CodespaceHost/GetFileContentAsync | (file read) | HOST root | Path (lexical-checked, symlink = host read) |
| CodespaceHost/InvokeSecondaryEditsAsync | python codeplan.py --config-file | HOST | ConfigJson/Task/Params |
| CodespaceHost/ConfigureEnvironmentAsync | codespaces configure | HOST | none |
| CodespaceHost/TerminateOryxTaskAsync | pkill -f oryx (docker) | container | none |
| VSCodeServerHost/StartRemoteServerAsync | vscode-dev-containers-cli start-server | HOST proc / container exec | VSCodeCommit, Quality, Extensions[], Version |
| VSCodeServerHost/ShutdownRemoteServerAsync | (server stop) | container | none |
| SshServerHost/StartRemoteServerAsync | installSSH.sh (docker exec) | container | UserPublicKey |
| JupyterServerHost/GetRunningServer | sh -c 'command -v jupyter' (docker exec) | container | none |
| OobControl/Shutdown | host shutdown | HOST | none (grpc 7 denied) |

## docker exec commands the agent builds (container-side, but is any string USER-influenced?)
### jupyter / oom / oryx / ssh flows — find the sh -c strings

### StartRemoteServerAsync commit->command flow (candidate injection)
  11643:    .field  private  string '<VSCodeCommit>k__BackingField'
  11667:           instance default string get_VSCodeCommit ()  cil managed 
  11675:	IL_0001:  ldfld string Codespaces.Grpc.VSCodeServerOptions::'<VSCodeCommit>k__BackingField'
  11677:    } // end of method VSCodeServerOptions::get_VSCodeCommit
  11681:           instance default void set_VSCodeCommit (string 'value')  cil managed 
  11690:	IL_0002:  stfld string Codespaces.Grpc.VSCodeServerOptions::'<VSCodeCommit>k__BackingField'
  11692:    } // end of method VSCodeServerOptions::set_VSCodeCommit
  11880:	.property instance string VSCodeCommit ()

## Interpolated-string commands with ldloc/ldarg (user data) near docker exec
  10593:	    IL_00eb:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted(string)
  10601:	    IL_0109:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted(string)
  10607:	    IL_011e:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted<class [System.Private.CoreLib]System.Exception> (!!0)
  15582:	  IL_0031:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted(string)
  15588:	  IL_0045:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted(string)
  15594:	  IL_0059:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted(string)
  16277:	  IL_0233:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted(string)
  16283:	  IL_0248:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted(string)
  16290:	  IL_0261:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted(string)
  16297:	  IL_027b:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted(string)
  18268:	  IL_0309:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted<int32> (!!0)
  18274:	  IL_031e:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted<int32> (!!0)
  18347:	  IL_0415:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted(string)
  18355:	  IL_0433:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted<bool> (!!0)
  18363:	  IL_0451:  call instance void class [System.Private.CoreLib]System.Runtime.CompilerServices.DefaultInterpolatedStringHandler::AppendFormatted(string)
