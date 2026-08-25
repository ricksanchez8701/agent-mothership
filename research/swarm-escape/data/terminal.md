# Agent terminal — every terminal input/write mechanism in the agent

## 1. gRPC methods mentioning terminal/input/write/keys (all ILs)
   1260 249642:Terminal

## 2. Client-streaming RPCs (client can push data) — search for IAsyncStreamReader in service impls

## 3. TerminalStreamRequest fields (is it REALLY empty?)
    .field  private static initonly  class [Google.Protobuf]Google.Protobuf.MessageParser`1<class Codespaces.Grpc.CodespaceHostService.V1.TerminalStreamRequest> _parser
    .field  private  class [Google.Protobuf]Google.Protobuf.UnknownFieldSet _unknownFields
    .field  public static initonly  class Codespaces.Grpc.CodespaceHostService.V1.TerminalStreamRequest/'<>c' '<>9'

## 4. EventStream event TYPES the agent emits (looks for command-ish types)
      8 "GitCredentialHelperCommandStrategy"
      5 "ExecuteFuseCommandAsync"
      4 "ExecuteGitCommandWithRetriesAsync"
      3 "TerminalStream"
      3 "SmbClientLoggingExecuteCommandAsync"
      3 "RunDevContainerStopCommandIfEnabledAsync"
      3 "postCreateCommand"
      2 "Output"
      2 "gitCommandFactory"
      2 "FullCommand"
      2 "ConfigCommandSuccess"
      1 "WriteSshCommands"
      1 "WarmupCommand"
      1 "Subscribed"
      1 "RunBlockingCommands"
      1 "PruneCommandSucceeded"
      1 "postAttachCommand"
      1 "OnStopCommandResult"
      1 "OnStopCommandPresent"
      1 "OnStopCommandFailureOuput"
      1 "ListCommandResult"
      1 "HasPostCheckoutCommand"
      1 "FsckCommandResult"
      1 "FsckCommandOutput"
      1 "FsckCommandError"

## 5. Any pty / pts / ioctl / TIOCSTI (terminal device writes)
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:100815:pty
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:104270:pty
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:108455:pty
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:108463:pty
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:108579:pty
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:108617:pty
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:108796:pty
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:108924:pty
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:109679:pty
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:109745:pty
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:109749:pty
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:110337:pty
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:110525:pty
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:110549:pty
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:110628:pty

## 6. ShellHelper::ExecuteAndGetOutput + ProcessStartInfo with RedirectStandardInput=true (interactive = writable stdin)
/workspaces/agent-mothership/research/beacon/tools/vsonline_core.il:30048:	IL_00bc:  callvirt instance void class [System.Diagnostics.Process]System.Diagnostics.ProcessStartInfo::set_RedirectStandardInput(bool)
/workspaces/agent-mothership/research/beacon/tools/vsonline_core.il:59178:	    IL_021d:  callvirt instance void class [System.Diagnostics.Process]System.Diagnostics.ProcessStartInfo::set_RedirectStandardInput(bool)
/workspaces/agent-mothership/research/beacon/tools/codespaces.il:187204:	IL_0022:  callvirt instance void class [System.Diagnostics.Process]System.Diagnostics.ProcessStartInfo::set_RedirectStandardInput(bool)
