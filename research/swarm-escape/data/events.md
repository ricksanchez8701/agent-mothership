# Agent events — the event bus as a command channel

## 1. Event types seen live in the wiretap (so far)
      1 "type": "Subscribed"
      1 "type": "EnvironmentConfigured"

## 2. EventStreamResponse.Type values the client code handles (IL)
     15 ldstr "ShellOutput"
      9 ldstr "OnStateChanged"
      8 ldstr "GitCredentialHelperCommandStrategy"
      5 ldstr "GrpcEventManager"
      5 ldstr "ExecuteFuseCommandAsync"
      4 ldstr "grpcEventManager"
      4 ldstr "ExecuteGitCommandWithRetriesAsync"
      3 ldstr "SmbClientLoggingExecuteCommandAsync"
      3 ldstr "RunDevContainerStopCommandIfEnabledAsync"
      3 ldstr "RawShellOutput"
      3 ldstr "postCreateCommand"
      3 ldstr "OnFileChangedAsync"
      3 ldstr "CommandOutputCount"
      2 ldstr "gitCommandFactory"
      2 ldstr "ErrorOutput"
      2 ldstr "ConfigCommandSuccess"
      1 ldstr "WriteSshCommands"
      1 ldstr "WarmupCommand"
      1 ldstr "VgsOutput"
      1 ldstr "UnknownEvent"

## 3. NotifyCodespaceOfClientActivity — what activities does the client send?
    .field  private static initonly  class [Google.Protobuf]Google.Protobuf.MessageParser`1<class Codespaces.Grpc.CodespaceHostService.V1.NotifyCodespaceOfClientActivityRequest> _parser
    .field  private  class [Google.Protobuf]Google.Protobuf.UnknownFieldSet _unknownFields
    .field public static literal  int32 ClientIdFieldNumber = int32(0x00000001)
    .field  private  string clientId_
    .field public static literal  int32 ClientActivitiesFieldNumber = int32(0x00000002)
    .field  private static initonly  class [Google.Protobuf]Google.Protobuf.FieldCodec`1<string> _repeated_clientActivities_codec
    .field  private initonly  class [Google.Protobuf]Google.Protobuf.Collections.RepeatedField`1<string> clientActivities_
    .field  public static initonly  class Codespaces.Grpc.CodespaceHostService.V1.NotifyCodespaceOfClientActivityRequest/'<>c' '<>9'

## 4. Is there any client->agent event (publish) path? IGrpcEventManager methods
  3409:    .field  private initonly  class Codespaces.Grpc.IGrpcEventManager '<GrpcEventManager>k__BackingField'
  3497:           instance default class Codespaces.Grpc.IGrpcEventManager get_GrpcEventManager ()  cil managed 
  3505:	IL_0001:  ldfld class Codespaces.Grpc.IGrpcEventManager Codespaces.Grpc.CodespaceHostServiceV1::'<GrpcEventManager>k__BackingField'
  3708:           instance default void '.ctor' (class Codespaces.Grpc.GrpcTraceSource grpcTrace, class [Microsoft.VsSaaS.Diagnostics]Microsoft.VsSaaS.Diagnostics.IDiagnosticsLogger logger, class Microsoft.CloudEnvironments.CLI.VmAgent.Monitoring.Monitors.ClientAutoSuspendMonitor clientAutoSuspendMonitor, class Codespaces.Grpc.IGrpcEventManager eventManager, class Microsoft.CloudEnvironments.CLI.VmAgent.Monitoring.IHeartBeatManager heartBeatManager, class [Microsoft.VisualStudio.VSOnline.Core]Microsoft.VisualStudio.VSOnline.Core.ClientUsageTracker clientUsage, class [Microsoft.VisualStudio.VSOnline.Core]Microsoft.VisualStudio.VSOnline.Core.Providers.IContainerInfoProvider containerInfoProvider, class [Microsoft.VisualStudio.VSOnline.Core]Microsoft.CloudEnvironments.CLI.Contracts.IInputProvider inputProvider, class [Microsoft.VisualStudio.VSOnline.Core]Microsoft.CloudEnvironments.CLI.Contracts.ICommandLine commandLine, class Microsoft.CloudEnvironments.CLI.VmAgent.Diagnostics.IActivityIdTracker activityIdTracker, [opt] class Codespaces.Grpc.IGrpcTerminalManager terminalManager, [opt] class [Microsoft.VisualStudio.VSOnline.Core]Microsoft.VisualStudio.VSOnline.Core.Contracts.ICurrentMetrics currentMetrics, [opt] class [Microsoft.VisualStudio.LiveShare.CoreContracts]Microsoft.Cascade.Contracts.IFileService fileService, [opt] class [Microsoft.VisualStudio.VSOnline.Core]Microsoft.VisualStudio.VSOnline.Core.CliSettings cliSettings, [opt] class Microsoft.CloudEnvironments.CLI.VmAgent.Builder.ITunnelHostManager tunnelHostManager, [opt] class [Microsoft.VisualStudio.VSOnline.Core]Microsoft.CloudEnvironments.CLI.Contracts.ILspServerManager lspServerManager)  cil managed 
  3738:	IL_003c:  call !!0 class [Microsoft.VisualStudio.Validation]Microsoft.Requires::NotNull<class Codespaces.Grpc.IGrpcEventManager> (!!0, string)
  3739:	IL_0041:  stfld class Codespaces.Grpc.IGrpcEventManager Codespaces.Grpc.CodespaceHostServiceV1::'<GrpcEventManager>k__BackingField'
  4123:	  IL_0046:  call instance class Codespaces.Grpc.IGrpcEventManager class Codespaces.Grpc.CodespaceHostServiceV1::get_GrpcEventManager()
  4128:	  IL_0053:  callvirt instance void class Codespaces.Grpc.IGrpcEventManager::AddSubscription(string, class [Grpc.Core.Api]Grpc.Core.IServerStreamWriter`1<class Codespaces.Grpc.CodespaceHostService.V1.EventStreamResponse>, class [System.Private.CoreLib]System.Threading.Tasks.TaskCompletionSource`1<object>)
  4994:	.property instance class Codespaces.Grpc.IGrpcEventManager GrpcEventManager ()
  4996:		.get instance default class Codespaces.Grpc.IGrpcEventManager Codespaces.Grpc.CodespaceHostServiceV1::get_GrpcEventManager () 

## 5. EventStream impl: what does it DO with the subscription id / any request fields?
             instance default class [System.Private.CoreLib]System.Threading.Tasks.Task EventStream (class Codespaces.Grpc.CodespaceHostService.V1.EventStreamRequest 'request', class [Grpc.Core.Api]Grpc.Core.IServerStreamWriter`1<class Codespaces.Grpc.CodespaceHostService.V1.EventStreamResponse> responseStream, class [Grpc.Core.Api]Grpc.Core.ServerCallContext context)  cil managed 
  	  IL_002d:  callvirt instance string class Codespaces.Grpc.CodespaceHostService.V1.EventStreamRequest::get_Id()
  	  IL_003b:  callvirt instance string class Codespaces.Grpc.CodespaceHostService.V1.EventStreamRequest::get_Id()
  	  IL_0053:  callvirt instance void class Codespaces.Grpc.IGrpcEventManager::AddSubscription(string, class [Grpc.Core.Api]Grpc.Core.IServerStreamWriter`1<class Codespaces.Grpc.CodespaceHostService.V1.EventStreamResponse>, class [System.Private.CoreLib]System.Threading.Tasks.TaskCompletionSource`1<object>)
