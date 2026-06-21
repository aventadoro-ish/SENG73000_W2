<!-- File generated with ChatGPT and modified by Matvey Regentov -->
# Raspberry Pi Deployment Setup

## Purpose

This document explains how to configure a development computer to copy the Raspberry Pi source code from the VS Code workspace to the project Raspberry Pi using SSH.

The deployment process uses:

* Raspberry Pi hostname: `LiF`
* Network address: `lif.local`
* Raspberry Pi user: `rita`
* Source directory: `RPi/LiF_SC`
* Remote deployment directory: `/home/rita/LiF_SC`
* VS Code build task: `Deploy LiF_SC to Raspberry Pi`

The Raspberry Pi and development computer do not need static IP addresses. The task connects using the Pi's `.local` hostname instead of its IP address.

## Repository structure

The expected repository structure is:

```text
project-root/
├── .git/
├── .gitignore
├── .vscode/
│   └── tasks.json
├── RPi/
│   └── LiF_SC/
└── documentation/
    └── RPi_Deployment_Setup.md
```

The `.vscode/tasks.json` file should be committed to the repository so that all team members receive the same deployment task.

## Shared VS Code task

Create or update:

```text
.vscode/tasks.json
```

Use the following configuration:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Deploy LiF_SC to Raspberry Pi",
            "type": "shell",
            "command": "scp",
            "args": [
                "-r",
                "${workspaceFolder}/RPi/LiF_SC/.",
                "rita@lif.local:/home/rita/LiF_SC/"
            ],
            "windows": {
                "args": [
                    "-r",
                    "${workspaceFolder}\\RPi\\LiF_SC\\.",
                    "rita@lif.local:/home/rita/LiF_SC/"
                ]
            },
            "problemMatcher": [],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "presentation": {
                "reveal": "always",
                "panel": "shared",
                "clear": true
            }
        }
    ]
}
```

The `${workspaceFolder}` variable allows each team member to clone the repository into a different location without modifying the task.

## Git configuration

The shared task should be committed, but personal VS Code settings and credentials should not be committed.

A suitable `.gitignore` configuration is:

```gitignore
# Ignore personal VS Code configuration
.vscode/*
!.vscode/tasks.json         # include shared build task
!.vscode/extensions.json

# SSH private keys and credentials
*.pem
*.key
id_rsa
id_ed25519
```

Do not store SSH private keys, passwords, or fixed IP addresses in the repository.

## Configuration performed once on the Raspberry Pi

The following configuration only needs to be performed once.

### Verify the hostname

On the Raspberry Pi, run:

```bash
hostname
```

The result should be:

```text
LiF
```

The Pi should then be reachable on the local network as:

```text
lif.local
```

Hostname matching is normally case-insensitive, but lowercase `lif.local` is used consistently in scripts and documentation.

### Enable SSH

Run:

```bash
sudo systemctl enable --now ssh
```

Verify that SSH is active:

```bash
systemctl status ssh
```

### Verify `.local` hostname support

Raspberry Pi OS normally uses Avahi to advertise its `.local` hostname.

Check its status:

```bash
systemctl status avahi-daemon
```

If it is not installed or active:

```bash
sudo apt update
sudo apt install avahi-daemon
sudo systemctl enable --now avahi-daemon
```

### Create the deployment directory

While logged in as `rita`, run:

```bash
mkdir -p /home/rita/LiF_SC
```

The deployment directory should remain owned by `rita`.

## Configuration required for each team member

Each team member must complete the following steps on their own computer.

### 1. Install or verify OpenSSH

On Windows PowerShell, run:

```powershell
ssh -V
scp
```

If `ssh` is not available, install the Windows OpenSSH Client through:

```text
Settings → System → Optional Features → Add an optional feature → OpenSSH Client
```

Linux and macOS systems normally include an SSH client. If necessary, install OpenSSH using the operating system's package manager.

### 2. Confirm that the Raspberry Pi is reachable

The computer and Raspberry Pi must be connected to the same local network.

Test hostname resolution:

```powershell
ping lif.local
```

Some networks or firewall configurations may block ping even when SSH works. The more important test is:

```powershell
ssh rita@lif.local
```

On the first connection, SSH may display a message similar to:

```text
The authenticity of host 'lif.local' can't be established.
Are you sure you want to continue connecting?
```

Verify that the device is the project Raspberry Pi, then enter:

```text
yes
```

Enter the shared `rita` account password when prompted.

### 3. Generate a personal SSH key

Each team member should generate their own key pair:

```powershell
ssh-keygen -t ed25519
```

When asked where to save the key:

```text
Enter file in which to save the key:
```

Press **Enter** to accept the default location:

```text
C:\Users\<username>\.ssh\id_ed25519
```

This creates two files:

```text
C:\Users\<username>\.ssh\id_ed25519
C:\Users\<username>\.ssh\id_ed25519.pub
```

The files have different purposes:

| File             | Purpose                                                     |
| ---------------- | ----------------------------------------------------------- |
| `id_ed25519`     | Private key. Keep this secret and never commit or share it. |
| `id_ed25519.pub` | Public key. This can be copied to the Raspberry Pi.         |

A passphrase may be added to protect the private key. If a passphrase is used, the user may need to enter it when connecting unless an SSH agent is configured.

### 4. Copy the public key to the Raspberry Pi

On Windows PowerShell, run:

```powershell
type $env:USERPROFILE\.ssh\id_ed25519.pub | ssh rita@lif.local "mkdir -p ~/.ssh && chmod 700 ~/.ssh && cat >> ~/.ssh/authorized_keys && chmod 600 ~/.ssh/authorized_keys"
```

Enter the `rita` password when prompted.

This command adds the team member's public key to:

```text
/home/rita/.ssh/authorized_keys
```

The private key remains on the team member's computer.

On Linux, macOS, Git Bash, or WSL, the following may be used instead:

```bash
ssh-copy-id rita@lif.local
```

### 5. Test key-based login

Run:

```powershell
ssh rita@lif.local
```

The connection should succeed without requesting the shared account password.

A private-key passphrase may still be requested if one was configured.

Exit the SSH session with:

```bash
exit
```

### 6. Clone or update the repository

For a new local copy:

```bash
git clone <repository-url>
```

For an existing local copy:

```bash
git pull
```

Open the repository root as the VS Code workspace.

The repository root must be the directory containing:

```text
.vscode/
RPi/
```

### 7. Run the deployment task

In VS Code, press:

```text
Ctrl+Shift+B
```

Because the deployment task is marked as the default build task, VS Code should run:

```text
Deploy LiF_SC to Raspberry Pi
```

The task copies the contents of:

```text
RPi/LiF_SC/
```

to:

```text
/home/rita/LiF_SC/
```

The copy operation is equivalent to:

```powershell
scp -r RPi/LiF_SC/. rita@lif.local:/home/rita/LiF_SC/
```

The VS Code terminal will display the transfer status and any SSH errors.

## Normal development workflow

The recommended workflow is:

1. Pull the latest repository changes.

2. Create or switch to the appropriate Git branch.

3. Modify and test the source code locally.

4. Commit or otherwise save important work.

5. Make sure no other team member is currently deploying.

6. Press `Ctrl+Shift+B` to copy the code to the Raspberry Pi.

7. Connect to the Pi if necessary:

   ```powershell
   ssh rita@lif.local
   ```

8. Run or test the deployed software from:

   ```text
   /home/rita/LiF_SC
   ```

The GitHub repository, not the Raspberry Pi, should be treated as the authoritative copy of the source code.

## Important limitations

### Both devices must be on the same local network

The `lif.local` address uses multicast DNS, or mDNS. It normally works when the Raspberry Pi and development computer are on the same LAN.

It may not work when:

* The laptop and Pi are connected to different networks.
* The network blocks client-to-client communication.
* The Wi-Fi network uses client isolation.
* mDNS traffic is blocked.
* The laptop is connected through a VPN that interferes with local networking.

A stable remote-access system such as Tailscale would be needed if the Pi must be accessed across unrelated networks.

### `scp` does not delete removed files

The task overwrites files that exist and adds new files, but it does not delete remote files that were removed locally.

For example, deleting this local file:

```text
RPi/LiF_SC/old_module.py
```

does not automatically delete:

```text
/home/rita/LiF_SC/old_module.py
```

Old files may therefore remain on the Raspberry Pi.

Do not automatically erase the remote directory unless it is confirmed that the directory contains only deployed source code and no generated data, configuration files, databases, logs, or test results.

### Concurrent deployments can overwrite files

All team members deploy to the same directory using the same Linux account. If two people deploy simultaneously, their files may overwrite one another.

Before deploying:

* Coordinate with the team.
* Avoid simultaneous deployments.
* Deploy known Git revisions whenever practical.
* Do not use files stored only on the Pi as the main copy of the project.

## Shared-account security considerations

All team members currently use the Raspberry Pi account:

```text
rita
```

Each team member should use a separate SSH key, even though the Linux account is shared.

Do not share private keys between team members.

The Pi may contain multiple public keys in:

```text
/home/rita/.ssh/authorized_keys
```

Public keys should include a recognizable comment, such as:

```text
matvey-laptop
```

This makes it easier to identify and revoke a specific person's access without changing every team member's configuration.

When someone no longer needs access, remove only their public-key line from `authorized_keys`.

The shared `rita` password should not be written in the repository or this document.

## Troubleshooting

### `Could not resolve hostname lif.local`

Confirm that both devices are on the same network.

Try:

```powershell
ping lif.local
```

On the Pi, check:

```bash
hostname
systemctl status avahi-daemon
```

Restart Avahi if necessary:

```bash
sudo systemctl restart avahi-daemon
```

### `Connection refused`

SSH may not be running on the Pi.

On the Pi:

```bash
sudo systemctl enable --now ssh
```

### `Permission denied`

First verify password-based login:

```powershell
ssh rita@lif.local
```

If password login works but key login does not, check the Pi permissions:

```bash
chmod 700 /home/rita/.ssh
chmod 600 /home/rita/.ssh/authorized_keys
chown -R rita:rita /home/rita/.ssh
```

### SSH still requests the account password

Confirm that the public key was added:

```bash
cat /home/rita/.ssh/authorized_keys
```

On Windows, confirm that the local files exist:

```powershell
Get-ChildItem $env:USERPROFILE\.ssh
```

The default key files should include:

```text
id_ed25519
id_ed25519.pub
```

To see which key SSH is trying to use:

```powershell
ssh -v rita@lif.local
```

### Host-key warning after the Pi is reinstalled

If the Raspberry Pi operating system is reinstalled, its SSH host key may change. SSH may display:

```text
WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!
```

Confirm that the Raspberry Pi was intentionally reinstalled before removing the old host entry.

Then run:

```powershell
ssh-keygen -R lif.local
```

Reconnect:

```powershell
ssh rita@lif.local
```

Verify and accept the new host key.

### VS Code cannot find the build task

Confirm that:

* The repository root is open in VS Code.
* `.vscode/tasks.json` exists.
* The JSON syntax is valid.
* The task is committed and present in the current Git branch.

Run the task through:

```text
Terminal → Run Build Task
```

or:

```text
Ctrl+Shift+B
```

## Setup checklist

### Raspberry Pi — performed once

* [ ] Hostname is `LiF`
* [ ] Pi is reachable as `lif.local`
* [ ] SSH service is enabled
* [ ] Avahi/mDNS service is active
* [ ] Shared user `rita` exists
* [ ] `/home/rita/LiF_SC` exists
* [ ] `.vscode/tasks.json` is committed to GitHub

### Each team member

* [ ] OpenSSH client is installed
* [ ] `ssh rita@lif.local` reaches the Pi
* [ ] A personal Ed25519 key pair has been generated
* [ ] The private key remains only on the team member's computer
* [ ] The public key has been added to the Pi
* [ ] Key-based SSH login works
* [ ] The latest repository version has been cloned or pulled
* [ ] The repository root is open in VS Code
* [ ] `Ctrl+Shift+B` successfully copies `RPi/LiF_SC` to the Pi
