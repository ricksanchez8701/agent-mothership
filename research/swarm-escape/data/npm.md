# Agent npm — mount files the agent might run (host vs container)

## Candidates in writable mount (/.codespaces/bin = host /.codespaces/agent/mount)

### file-syncer.js
- in-mount: yes
- IL references: /workspaces/agent-mothership/research/beacon/tools/codespaces.il:1 /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:1 
- first reference context:
    /workspaces/agent-mothership/research/beacon/tools/codespaces.il:186989:	IL_0013:  ldstr "file-syncer.js"
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:9003:	IL_0005:  ldstr "file-syncer/dist/src/file-syncer.js"

### file-syncer-bridge.js
- in-mount: yes
- IL references: /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:1 
- first reference context:
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:9032:	IL_0006:  ldstr "file-syncer-bridge.js"

### codespaceStatusTool.js
- in-mount: yes
- IL references: /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:2 
- first reference context:
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:8497:	IL_000f:  ldstr "codespaceStatusTool.js"
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:8511:	IL_0006:  ldstr "codespaceStatusTool.js"

### gitcredential_github.sh
- in-mount: yes
- IL references: /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:2 
- first reference context:
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:9046:	IL_0006:  ldstr "gitcredential_github.sh"
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:9060:	IL_0006:  ldstr "gitcredential_github.sh"

### start_jupyter_server.sh
- in-mount: yes
- IL references: /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:2 
- first reference context:
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:6414:    .field private static literal  string JupyterLabLauncherScriptFileName = "start_jupyter_server.sh"
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:7941:	IL_0006:  ldstr "start_jupyter_server.sh"

### installSSH.sh
- in-mount: no
- IL references: /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:4 
- first reference context:
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:6412:    .field private static literal  string SshScriptFileName = "installSSH.sh"
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:7884:	IL_000b:  ldstr "installSSH.sh"
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:7927:	IL_0006:  ldstr "installSSH.sh"
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:8321:	IL_0006:  ldstr "installSSH.sh"

### sourcer.sh
- in-mount: yes
- IL references: NONE

### mount_data_disk.sh
- in-mount: yes
- IL references: /workspaces/agent-mothership/research/beacon/tools/codespaces.il:2 
- first reference context:
    /workspaces/agent-mothership/research/beacon/tools/codespaces.il:129377:    .field private static literal  string DataDiskMountScript = "/.codespaces/agent/bin/mount_data_disk.sh"
    /workspaces/agent-mothership/research/beacon/tools/codespaces.il:138068:	  IL_0103:  ldstr "/.codespaces/agent/bin/mount_data_disk.sh"

### backup_data_disk_images.sh
- in-mount: yes
- IL references: /workspaces/agent-mothership/research/beacon/tools/codespaces.il:2 
- first reference context:
    /workspaces/agent-mothership/research/beacon/tools/codespaces.il:129378:    .field private static literal  string DataDiskBackUpScript = "/.codespaces/agent/bin/backup_data_disk_images.sh"
    /workspaces/agent-mothership/research/beacon/tools/codespaces.il:138319:	  IL_0068:  ldstr "/.codespaces/agent/bin/backup_data_disk_images.sh"

### smbclientlogs.sh
- in-mount: yes
- IL references: /workspaces/agent-mothership/research/beacon/tools/codespaces.il:2 
- first reference context:
    /workspaces/agent-mothership/research/beacon/tools/codespaces.il:94127:	  IL_0053:  ldstr "smbclientlogs.sh"
    /workspaces/agent-mothership/research/beacon/tools/codespaces.il:107199:	  IL_006a:  ldstr "smbclientlogs.sh"

### installCWTools.sh
- in-mount: no
- IL references: /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:4 
- first reference context:
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:6413:    .field private static literal  string CWToolsScriptFileName = "installCWTools.sh"
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:7899:	IL_000b:  ldstr "installCWTools.sh"
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:7913:	IL_0006:  ldstr "installCWTools.sh"
    /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:8336:	IL_0006:  ldstr "installCWTools.sh"

## KEY QUESTION: does anything run these from the MOUNT path on the HOST?
- VmCLICopyFolder getter (vsonline_common.il) decides the mount path;
- grep for get_VmCLICopyFolder call sites to see what runs from it.
  /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:8131:           instance default string get_VmCLICopyFolder ()  cil managed 
  /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:8142:    } // end of method FilePathProviderBase::get_VmCLICopyFolder
  /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:8424:	IL_0001:  callvirt instance string class Microsoft.VisualStudio.VSOnline.Common.BaseClasses.FilePathProviderBase::get_VmCLICopyFolder()
  /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:8524:	IL_0001:  callvirt instance string class Microsoft.VisualStudio.VSOnline.Common.BaseClasses.FilePathProviderBase::get_VmCLICopyFolder()
  /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:8539:	IL_0001:  callvirt instance string class Microsoft.VisualStudio.VSOnline.Common.BaseClasses.FilePathProviderBase::get_VmCLICopyFolder()
  /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:9045:	IL_0001:  callvirt instance string class Microsoft.VisualStudio.VSOnline.Common.BaseClasses.FilePathProviderBase::get_VmCLICopyFolder()
  /workspaces/agent-mothership/research/beacon/tools/vsonline_common.il:9595:		.get instance default string Microsoft.VisualStudio.VSOnline.Common.BaseClasses.FilePathProviderBase::get_VmCLICopyFolder () 
  /workspaces/agent-mothership/research/beacon/tools/codespaces.il:203916:	  IL_0096:  callvirt instance string class [Microsoft.VisualStudio.VSOnline.Common]Microsoft.VisualStudio.VSOnline.Common.BaseClasses.FilePathProviderBase::get_VmCLICopyFolder()
  /workspaces/agent-mothership/research/beacon/tools/codespaces.il:204351:	  IL_0095:  callvirt instance string class [Microsoft.VisualStudio.VSOnline.Common]Microsoft.VisualStudio.VSOnline.Common.BaseClasses.FilePathProviderBase::get_VmCLICopyFolder()
  /workspaces/agent-mothership/research/beacon/tools/codespaces.il:204837:	  IL_0062:  callvirt instance string class [Microsoft.VisualStudio.VSOnline.Common]Microsoft.VisualStudio.VSOnline.Common.BaseClasses.FilePathProviderBase::get_VmCLICopyFolder()
