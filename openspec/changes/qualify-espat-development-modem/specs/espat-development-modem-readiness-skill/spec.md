## ADDED Requirements

### Requirement: Project-local readiness skill
The repository SHALL provide a small project-local skill named `espat-development-modem-readiness` that guides repeatable ESP-AT development-modem qualification and treats the single repository qualification tool as the executable source of truth.

#### Scenario: Operator invokes the skill
- **WHEN** the skill is selected for modem readiness work
- **THEN** it directs the operator through prerequisite checks, discovery, safe serial qualification, baseline verification, Wi-Fi/TCP qualification, evidence review, and result interpretation using the repository tool

### Requirement: Skill and tooling remain synchronized
The skill SHALL reference the repository-relative qualification command and output rather than duplicating protocol logic, and SHALL identify its safe modes, required inputs, and minimum evidence.

#### Scenario: Script behavior changes
- **WHEN** the qualification tool interface or evidence format is changed
- **THEN** skill validation detects or review tasks reconcile any stale command or output reference before qualification is accepted

### Requirement: Safe authorization boundaries
The skill SHALL state and enforce that flashing, erasing, `AT+RESTORE`, package installation, and access to unrelated serial devices require authority not granted by routine qualification, and it SHALL keep STM32 USART1 connection and implementation outside its workflow.

#### Scenario: Destructive recovery appears necessary
- **WHEN** ESP-AT is missing or unresponsive and recovery would require firmware mutation or restore
- **THEN** the skill stops, preserves diagnostic evidence, and requests explicit user authorization before any such action

### Requirement: Secret-safe guided operation
The skill SHALL support partial qualification without credentials and SHALL use only the repository tool's ephemeral, non-echoing input for Wi-Fi credentials. It SHALL warn against placing secrets in tracked files, command arguments, or retained evidence and SHALL require a secret review before evidence is retained. It SHALL state that the change remains active until Wi-Fi/TCP checks pass or the user explicitly accepts partial qualification.

#### Scenario: Credentials are unavailable
- **WHEN** the operator cannot or does not provide Wi-Fi credentials
- **THEN** the skill completes all non-secret stages and marks association, IP, and dependent TCP checks not-run

#### Scenario: Evidence is prepared for retention
- **WHEN** a qualification record may be committed
- **THEN** the skill invokes or instructs the repository secret check and refuses to describe the record as ready if credential material is detected

### Requirement: Environment and failure reporting
The skill SHALL check for required host capabilities without installing packages and SHALL distinguish missing prerequisites, permission problems, busy ports, ambiguous devices, modem failures, Wi-Fi failures, and remote TCP endpoint failures.

#### Scenario: Host dependency is missing
- **WHEN** a required executable, permission, or device access capability is unavailable
- **THEN** the skill reports the exact prerequisite and stops the affected stage without installing or modifying host software
