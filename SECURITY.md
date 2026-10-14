# Security Policy

This is open source firmware for personal use on hardware you own. It is not a
certified Matter product and ships on shared test credentials. See
[Certification](docs/CERTIFICATION.md) for details.

## Reporting

For a functional bug, including a fail-safe or reliability issue, open a normal
issue. For something genuinely exploitable, an attacker-reachable flaw, a
credential or key exposure, or a commissioning or OTA weakness, use GitHub's
[private vulnerability reporting](https://github.com/automatous-io/shelly-1-gen4-matter-thread/security/advisories/new) on this repository so it can be triaged before
disclosure.

## How issues are handled

Fixes and disclosure happen in the open: a public issue, a pull request, and a
note in CHANGELOG.md and the release notes for the affected firmware. Severity is
assessed by the maintainer. A formal advisory or CVE is reserved for
vulnerabilities that are remotely exploitable or expose credentials or keys.
Reliability and fail-safe bugs are fixed and documented as normal issues, not
advisories.

## Supported versions

The latest released version of the firmware receives fixes. Older versions, and
the earlier per-variant releases that are no longer built, do not.
