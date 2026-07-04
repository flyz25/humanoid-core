# Security Policy

## Supported Versions

humanoid-core is currently pre-1.0 production framework infrastructure.

| Version | Supported |
| --- | --- |
| `0.1.x-alpha` | Yes |
| Older snapshots | No |

Security fixes are applied to the active development line until the first stable
release policy is published.

## Reporting Vulnerabilities

Do not open public issues for vulnerabilities, unsafe robot-control defects, or
security-sensitive dependency problems.

Report privately to the maintainers using the repository security advisory
process when available. If advisories are not enabled, contact the maintainers
through the private channel documented by the project owner.

Include:

- affected commit or version,
- reproduction steps,
- build configuration,
- SDK version,
- robot model, when relevant,
- impact assessment,
- suggested mitigation, if known.

## Private Disclosure Process

1. Maintainers acknowledge receipt privately.
2. Maintainers assess severity and reproducibility.
3. A private fix branch is prepared when needed.
4. Release timing is coordinated with affected downstream users.
5. Public disclosure occurs after a fix or mitigation is available.

## Security Expectations

- Vendor SDK headers must remain isolated behind SDK wrappers.
- Secrets, credentials, and private keys must not be committed.
- Logs attached to issues must be reviewed for sensitive data.
- Build scripts must not download or execute remote shell content.
- New dependencies require review for license, maintenance, and security risk.

## Robot Safety Notice

Unsafe robot-control behaviour can create physical safety risk. Issues that may
cause unintended motion, failure to stop, incorrect emergency-stop behaviour, or
unsafe hardware command sequencing must be reported responsibly through the
private disclosure process.

Do not publish reproduction steps that could encourage unsafe operation of a
physical robot before maintainers have assessed the issue.
