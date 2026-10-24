# Code signing policy

Status: application submitted on 24 October 2026; website submission confirmed.
A decision is pending. SignPath Foundation has not accepted this project, no
signing service is configured, and current pilot DLLs are unsigned.

## Intended process

The DLL is built from the public repository on a GitHub-hosted Windows runner.
Native unit tests, OpenXR protocol tests, a real Windows DirectInput proxy smoke
test and VERSIONINFO checks must pass first. The uploaded artifact contains only
the project's own `dinput8.dll`. Game and Meta binaries are never submitted.

Signing is requested only by a manual workflow run on `main`. The SignPath
production signing policy must require a human approver for every release.
The returned DLL must have a valid trusted Authenticode signature, a timestamp
and the configured signer thumbprint before it is offered as a signed artifact.
This is not a claim that every security product will allow the file.

## Roles to configure before applying

| Role | Account |
| --- | --- |
| Committer / maintainer | [MR_Scarface (@MR-Scarface-VR)](https://github.com/MR-Scarface-VR) |
| Reviewer | MR_Scarface (@MR-Scarface-VR) — initial single-maintainer project |
| Signing approver | MR_Scarface (@MR-Scarface-VR) — human approval in SignPath |

Enable MFA for repository and SignPath access. Never put tokens in the repository.
See [privacy](PRIVACY.md) and [setup instructions](signing/SETUP.md).

After acceptance, update the status and add the required attribution:

> Free code signing provided by [SignPath.io](https://about.signpath.io), certificate by [SignPath Foundation](https://signpath.org).

This future attribution is a draft, not a statement of current sponsorship.
