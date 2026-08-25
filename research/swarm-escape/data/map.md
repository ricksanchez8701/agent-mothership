# Agent map — writable-from-container vs executed-by-agent

## Container mountinfo (writable sources)

## FilePathProvider getters (paths the agent computes) — from IL
  get_AzCopyTeardownScratchDirectoryPath ()
  get_BaseFolder ()
  get_CacheFolder ()
  get_CaptureRangesOutputFile ()
  get_ClonedRepositoryFolder ()
  get_CodeplanConfigPath ()
  get_CodeplanExecutablePath ()
  get_CodeplanPythonPath ()
  get_CodeplanTaskPath ()
  get_CodespacesGrpcSocketContainerPath ()
  get_CodespacesLogsContainerPath ()
  get_CodespacesLogsVMPath ()
  get_CodespaceStatusToolAgentFile ()
  get_CodespaceStatusToolSharedFile ()
  get_ContainerBaseFolder ()
  get_ContainerClonedRepositoryFolder ()
  get_ContainerDevContainersCLICacheFolder ()
  get_ContainerDockerFolder ()
  get_ContainerDotFilesCloneFolder ()
  get_ContainerExecutableFolder ()
  get_ContainerGitCredentialHelperScriptPath ()
  get_ContainerInternalFolder ()
  get_ContainerOverrideFilePath ()
  get_ContainerPersistedDockerConfigFolder ()
  get_ContainerPersistedJupyterLabLogFile ()
  get_ContainerPersistedShareFolder ()
  get_ContainerSessionPath ()
  get_ContainerSharedFolder ()
  get_ContainerSshFolder ()
  get_ContainerTempFolder ()
  get_ContainerVSCodeSettingsFolderFullPath ()
  get_ContainerWorkspaceFolder ()
  get_CopilotWorkspaceFileSyncerConfigPath ()
  get_CopilotWorkspaceFileSyncerLogPath ()
  get_CopilotWorkspaceFileSyncerPath ()
  get_CreationLogFile ()
  get_CustomDevContainerJsonPath ()
  get_CustomDevContainerRelativePath ()
  get_CWToolsScriptDestinationFilePath ()
  get_CWToolsScriptFilePath ()

## Writable dirs we have confirmed (from sessions)
  - /workspaces/.codespaces/shared  writable=yes
  - /tmp  writable=yes
  - /workspaces/agent-mothership  writable=yes
  - /.codespaces/bin  writable=yes

## What the agent RUNS that we can WRITE
  (manually curated from sessions + this swarm)
  - /.codespaces/bin/*.sh  (wrappers, now forked with ext-beacon)
  - workspace repo .git/config -> credential.helper + hooksPath (planted)
  - /workspaces/.codespaces/shared/merged_devcontainer.json + user-secrets-envs.json (read by devcontainer CLI)
  - /root/.codespaces/shared/beacon.js (launched by ext-beacon when a wrapper fires)

## Host dirs NOT writable from container (agent runs from here)
  - /.codespaces/agent/bin
  - /.codespaces/agent/mount
  - /usr/local/lib/node_modules
  - /usr/bin
