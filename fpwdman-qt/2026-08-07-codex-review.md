# fpwdman-qt security review

Date: 2026-08-07  
Reviewer: Codex  
Scope: read-only review of the application, modern and legacy vault formats, GUI secret handling, build configuration, and existing tests.

## Executive summary

The current implementation has several good controls: OS-backed random generation, per-file salts, PBKDF2-HMAC-SHA256 with 600,000 iterations, separate encryption and MAC keys, encrypt-then-MAC with verification before decryption, constant-time tag comparison, atomic writes through `QSaveFile`, masked password fields, timed clipboard clearing, and best-effort wiping of some derived byte buffers.

The most urgent concrete defect is that an untrusted modern vault controls its PBKDF2 iteration count. The value is used before authentication and has no upper bound, so a tiny file can freeze the application for an extremely long time. The input paths also contain unbounded reads and allocations, including a legacy header-controlled `reserve()` that occurs before decryption. These should be fixed before changing the vault's cryptographic design.

The largest architectural risk is the use of the project-specific SBC block cipher for newly written vaults. No practical break was established by this review, but the implementation does not have the assurance of a standard cipher and maintained cryptographic library. A new standard-AEAD format should be designed as a versioned migration, while preserving legacy and FPMQ1 read compatibility.

## Priority 0: bound attacker-controlled work and memory

### P0.1 Reject untrusted PBKDF2 iteration counts before deriving keys

Severity: High (local denial of service)  
Primary file: `src/crypto/PasswordStore.cpp`  
Relevant code: `decodeModern()` around lines 273-305; `deriveKeys()` around lines 152-160.

Current behavior:

- `decodeModern()` parses `iters` from the file using `toInt()`.
- It checks only that the result is greater than zero.
- The value is passed to PBKDF2 before the file's MAC can be checked.
- PBKDF2 produces 96 bytes, so it performs three blocks of work. A value near `INT_MAX` therefore causes billions of HMAC operations on the GUI thread.

Recommended implementation:

1. Treat FPMQ1 parameters as part of the version specification. For an FPMQ1 file, require `iters == 600000` rather than accepting arbitrary positive values.
2. If compatibility with previously generated nonstandard FPMQ1 files is actually required, define explicit minimum and maximum constants and reject anything outside that narrow range before calling `deriveKeys()`. Do not use a broad maximum merely because the integer type permits it.
3. Parse the decimal value strictly: reject empty values, signs, overflow, trailing characters, and duplicates.
4. Return `CorruptFile`, not `WrongPassphrase`, for an invalid work factor.
5. A future format may permit selectable KDF parameters, but each version must still impose hard CPU and memory ceilings before doing KDF work.

Acceptance criteria:

- An FPMQ1 file with `iters=1`, `iters=599999`, `iters=600001`, `iters=2147483647`, a negative value, overflow, or non-decimal suffix is rejected quickly without invoking the expensive KDF.
- A normal `iters=600000` vault continues to open.
- Existing golden/round-trip files remain compatible.
- Unit tests do not rely on wall-clock timing alone; expose or structure parameter validation so tests can prove rejection occurs before PBKDF2.

Suggested tests in `test/StoreTest.cpp`:

- `ModernRejectsUnexpectedIterationCount`
- `ModernRejectsIterationOverflow`
- `ModernRejectsMalformedIterationCount`
- `ModernRejectsDuplicateIterationField`

### P0.2 Add hard input and decompression limits

Severity: Medium to High (memory exhaustion or process termination)  
Primary files: `src/crypto/PasswordStore.cpp`, `src/sbc/sbc_legacy.cpp`  
Relevant code:

- `openFile()` calls `QFile::readAll()` around `PasswordStore.cpp:349-354`.
- `decodeModern()` calls `split('\n')` and repeatedly appends to `body` around `PasswordStore.cpp:273-298`.
- `parseEncFile()` calls `r64.reserve(numPrintChar)` using an untrusted header around `sbc_legacy.cpp:143-160`.
- `sbcUncompress()` allocates `rawLen` bytes from an embedded 32-bit value around `sbc_legacy.cpp:108-138`.

Current risks:

- A large file is copied several times in memory.
- A tiny legacy file can advertise a huge `numPrintChar` and trigger a huge allocation before any password-dependent operation.
- A valid or crafted legacy payload can advertise up to roughly 4 GiB of decompressed output.
- `std::bad_alloc` is not translated to the application's error types and may terminate the operation or process.

Recommended implementation:

1. Define documented resource limits in one place. A 32 MiB or 64 MiB maximum encrypted vault is likely generous, but choose it from expected real databases rather than blindly copying this suggestion.
2. In `openFile()`, inspect `QFileInfo::size()` before `readAll()` and reject negative, unavailable, or over-limit sizes. Still verify the number of bytes actually read.
3. Avoid `fileText.split('\n')` for the modern format. Parse a bounded number of header lines and then decode the remaining body incrementally or at least from a bounded slice.
4. In `parseEncFile()`, require `numPrintChar` to be within the global input limit and consistent with the actual remaining input before calling `reserve()`.
5. Validate `numFullChar` as well as `numPrintChar`; do not retain a parsed length that has no enforced relationship to the body.
6. In `sbcUncompress()`, reject `rawLen` above a separate decompressed-XML limit before allocating. Consider a ratio limit as additional defense, but never use a ratio limit instead of an absolute limit.
7. Add limits for XML entry count and individual field lengths so a small compressed document cannot create excessive Qt objects.
8. Catch `std::bad_alloc` at the file boundary and report a controlled `CorruptFile` or a dedicated resource-limit error. Up-front checks remain the main defense.

Acceptance criteria:

- Oversized modern and legacy files are rejected before `readAll()`.
- A tiny legacy file with a huge first header number is rejected without a large allocation.
- A legacy payload with an excessive `rawLen` is rejected before allocating the output vector.
- Boundary-sized valid files still open.
- Error dialogs distinguish an unsupported/oversized file from a wrong passphrase where practical.

Suggested tests:

- `OpenRejectsFileOverSizeLimit`
- `LegacyRejectsOversizedArmorLengthBeforeReserve`
- `LegacyRejectsOversizedRawLengthBeforeInflate`
- `ModernRejectsExcessiveEntryCount`
- Boundary tests at limit minus one, limit, and limit plus one.

### P0.3 Make modern header parsing strict and unambiguous

Severity: Medium (parser ambiguity and support for resource attacks)  
Primary file: `src/crypto/PasswordStore.cpp`  
Relevant code: `headerField()` around lines 78-96 and `decodeModern()` around lines 273-305.

Current behavior:

- Header lines are classified using substring searches such as `contains("salt=")`.
- Duplicate and reordered headers are accepted, with later values silently replacing earlier ones.
- KDF and MAC algorithm names are not validated.
- Salt and tag lengths are checked only for non-emptiness.
- `QByteArray::fromBase64()` is used permissively.
- The ciphertext/body has no explicit maximum.

Recommended implementation:

1. Specify the exact FPMQ1 grammar in a comment or format document.
2. Require exactly three logical header lines: magic, KDF parameters, and MAC parameters, followed by the Base64 body.
3. Require exact supported algorithm identifiers.
4. Reject unknown, missing, and duplicate fields.
5. Decode using strict Base64 error handling (`AbortOnBase64DecodingErrors` where available in the supported Qt version).
6. Require a 16-byte salt and 32-byte HMAC tag.
7. Require nonempty ciphertext whose decoded length is a multiple of 128 bytes and below the configured maximum.
8. For the next format, authenticate a canonical serialization of the version and parameters as AEAD associated data. Note that authenticating parameters does not replace pre-KDF resource limits, because authentication itself requires a derived key.

Acceptance criteria:

- Whitespace tricks, stray Base64 characters, duplicate fields, unknown algorithms, wrong salt/tag lengths, and unexpected header ordering are rejected deterministically.
- The checked-in FPMQ1 example and files written by the current encoder remain readable.
- Tests cover every parser rejection class.

## Priority 1: replace custom cryptography for newly written vaults

### P1.1 Design and implement an FPMQ2 standard-AEAD container

Severity: High architectural risk; no practical SBC break was established  
Primary files: `src/crypto/PasswordStore.cpp`, `src/crypto/PasswordStore.h`, `src/sbc/*`, `CMakeLists.txt`, tests and format documentation.

Current behavior:

- Both legacy and modern containers use the project-specific SBC block cipher.
- FPMQ1 uses SBC-CBC with a zero IV, a randomized plaintext prefix, and a separate HMAC-SHA256 tag.
- The encrypt-then-MAC ordering and separate keys are good, but they cannot establish the confidentiality strength of SBC itself.

Recommended direction:

1. Create a versioned `FPMQ2` format using a standard maintained implementation of either:
   - XChaCha20-Poly1305, preferably through libsodium; or
   - AES-256-GCM through a well-maintained platform/library API.
2. Prefer Argon2id for passphrase derivation. If project constraints require PBKDF2, retain at least PBKDF2-HMAC-SHA256/600,000 and document the constraint. Current OWASP guidance prefers Argon2id and lists PBKDF2-HMAC-SHA256/600,000 when PBKDF2 is required.
3. Calibrate KDF parameters for the slowest supported target and store bounded, versioned parameters in the header.
4. Use a fresh random salt and nonce per save. Enforce exact lengths and nonce uniqueness requirements dictated by the selected AEAD.
5. Authenticate the canonical header as associated data.
6. Continue to read legacy and FPMQ1 files, but write FPMQ2 on the next successful save. Never overwrite the only good copy until the new file is fully committed and verified.
7. Keep legacy SBC code read-only and clearly separated from new-write code.
8. Obtain an independent cryptographic design review before declaring FPMQ2 stable. Format code should include test vectors generated independently from the application.

Migration considerations:

- Decide whether an opened FPMQ1 file is migrated automatically on save or only after an explicit prompt.
- Coordinate migration with the backup policy below; otherwise the old SBC-encrypted copy will remain beside the new vault.
- Preserve Unicode passphrase semantics exactly or explicitly version normalization behavior. Changing UTF-8 conversion or Unicode normalization can lock users out.
- Provide a recovery procedure if commit succeeds but subsequent verification fails.

Acceptance criteria:

- New vaults are written only as FPMQ2.
- Legacy and FPMQ1 fixtures remain readable.
- A modified header, nonce, ciphertext, or tag is rejected.
- Wrong-passphrase and corrupt-file behavior does not expose a decryption oracle.
- Independent known-answer tests cover the selected KDF and AEAD.
- Fuzz tests cover the complete FPMQ2 decoder.

Reference guidance:

- OWASP Cryptographic Storage Cheat Sheet: standard algorithms and authenticated modes should be preferred; custom algorithms should not be used.
- OWASP Password Storage Cheat Sheet: Argon2id is preferred; PBKDF2-HMAC-SHA256 with 600,000 iterations is the recommendation when PBKDF2 is required.

## Priority 1: fix backup and passphrase-rotation semantics

### P1.2 Do not silently retain an old-key or legacy-encrypted vault

Severity: Medium  
Primary file: `src/crypto/PasswordStore.cpp`  
Relevant code: `saveFile()` around lines 371-390; tests around `Store.SaveFileCreatesBackupAndAtomicFile`.

Current behavior:

- Before every save, the current file is copied byte-for-byte to `path + ".bak"`.
- Existing backup removal and new backup copy results are ignored.
- Changing the master passphrase leaves the backup decryptable with the old passphrase.
- Migrating a legacy file leaves the weak legacy-encrypted original in the backup.

Security and product decision required:

Backups are valuable for availability, so silently deleting all backups is not automatically correct. The application must define and communicate whether rollback or passphrase revocation takes precedence. The current silent behavior gives users neither visibility nor control.

Recommended implementation:

1. Make backup behavior an explicit policy, documented in the UI and format documentation.
2. At minimum, warn during legacy migration or master-passphrase rotation that the `.bak` copy remains protected by the old scheme/passphrase.
3. Prefer creating a backup that is re-encrypted under the current format and current passphrase when feasible, rather than copying old bytes.
4. Check and handle every `QFile::remove()` and `QFile::copy()` result. Do not claim backup success if either operation failed.
5. Preserve atomicity: the main vault must remain intact if backup creation or new-file commit fails.
6. Consider bounded, opt-in versioned backups instead of a single implicit sidecar.
7. Do not promise secure deletion. SSD wear leveling, journaling, snapshots, cloud synchronization, and antivirus/history tools can retain old copies.
8. Ensure newly created vault and backup files receive restrictive user-only permissions where the target OS supports them; verify behavior on Windows and Unix rather than assuming defaults.

Acceptance criteria:

- Backup creation failures are surfaced and do not silently proceed under a false assurance.
- A passphrase rotation test explicitly verifies the chosen old-backup behavior.
- A legacy-to-modern migration test verifies the chosen legacy-backup behavior.
- Permission tests or documented platform checks cover both main and backup files.

## Priority 2: improve master-passphrase policy

### P2.1 Replace the six-character floor with useful strength guidance

Severity: Medium (offline guessing risk)  
Primary files: `src/MainWindow.cpp`, `src/PreferencesDialog.h`, `src/PreferencesDialog.cpp`, `src/ChangeMasterPassphraseDialog.cpp`.

Relevant code:

- `kMinMasterFloor = 6` in `MainWindow.cpp` around line 34.
- Default `minMasterLength = 8` in `PreferencesDialog.h` around line 18.
- Preference range permits values as low as four, although the application later applies the floor of six.

Current risk:

An attacker who steals a vault can guess passphrases offline. PBKDF2 increases guess cost but does not make a short or common passphrase strong. A six- or eight-character minimum may be interpreted by users as an adequate recommendation.

Recommended implementation:

1. Recommend at least four randomly selected words or approximately 14-16 characters, without imposing composition rules that encourage predictable substitutions.
2. Integrate a well-maintained strength estimator or, at minimum, reject a bundled list of extremely common passwords and explain why weak choices are dangerous.
3. Distinguish a safety floor from the recommended target in UI text.
4. Do not silently truncate passphrases. Preserve full Unicode input and test long passphrases.
5. If the minimum remains configurable, prevent configuration from dropping below the actual safety floor and remove the misleading 4-5 range from the preference widget.
6. Consider offering generation of a multiword master passphrase, but never store it outside the encrypted vault workflow.

Acceptance criteria:

- The UI no longer implies that six characters is adequate.
- Common/weak passphrases produce a clear warning or rejection according to the chosen policy.
- Long and non-ASCII passphrases round-trip correctly.
- Existing vaults with short passphrases remain openable; stronger policy applies when creating or changing a passphrase, not when decrypting old data.

## Priority 2: reduce secret exposure during an unlocked session

### P2.2 Add a real lock action and shorten/configure idle exposure

Severity: Low to Medium, depending on the local-device threat model  
Primary files: `src/MainWindow.cpp`, `src/MainWindow.h`, `src/SiteDatabase.h`, dialog classes.

Current behavior:

- The complete database and master passphrase stay in ordinary `QString`/`std::vector` objects for the unlocked session.
- `SiteDatabase::reset()` calls `clear()`, which does not guarantee memory overwriting.
- The idle timeout is fixed at one hour and closes the application rather than providing a reusable locked state.
- There is no immediate manual lock command.

Recommended implementation:

1. Add a prominent Lock action and shortcut that clears the UI, removes the decrypted database, clears a still-owned clipboard value, and requires the passphrase to reopen.
2. Make the idle timeout configurable with a shorter secure default, including an option to lock when the workstation/session is locked if Qt/platform hooks permit it.
3. Expand activity tracking to relevant input events such as wheel, touch, tablet, and application activation where appropriate.
4. Minimize passphrase copies and scope temporary values tightly. Avoid claims that `QString::clear()` securely erases data.
5. For byte buffers that can be uniquely owned, continue explicit wiping using a platform primitive such as `SecureZeroMemory`/`explicit_bzero` or a vetted library abstraction.
6. Consider disabling crash dumps for release builds or documenting that process dumps and page files can contain decrypted secrets. This requires platform-specific evaluation.

Acceptance criteria:

- Locking removes all entries and secret text from visible widgets and application-owned clipboard state.
- Unlock requires re-reading/decrypting the vault rather than trusting a cached master passphrase.
- Idle-lock behavior has automated state tests where practical.
- Unsaved-change behavior is explicitly designed: locking must not silently discard edits unless the user chose that policy.

### P2.3 Document clipboard limitations and reduce retained copies

Severity: Low  
Primary files: `src/ClipboardUtil.h`, `src/ViewSiteEntry.cpp`, `src/MainWindow.cpp`.

Current behavior:

- Copied passwords are cleared after a bounded interval if the clipboard still contains the same text.
- The last password is also stored in a static `QString`.
- Each single-shot timer lambda captures another copy until it fires or the application exits.
- Clipboard managers, remote-desktop software, accessibility tools, and other processes may retain clipboard history despite later clearing.

Recommended implementation:

1. Use one managed timer and one current clipboard-secret value instead of creating a new lambda capture for every copy.
2. On a new copy, cancel/restart the existing timer and clear/replace the previous application-owned value.
3. Clear clipboard state on manual lock as well as shutdown.
4. Add concise UI/help text stating that auto-clear cannot erase third-party clipboard history.
5. Consider an optional direct auto-type workflow only after a separate threat-model and accessibility review; it should not be treated as automatically safer.

Acceptance criteria:

- Repeated copying does not leave multiple pending application-owned copies.
- Lock and shutdown clear the clipboard only when it still contains the application's value, preserving unrelated newer user content.
- Tests cover copy, recopy, external clipboard replacement, timeout, lock, and shutdown.

## Priority 3: testing, fuzzing, and build hardening

### P3.1 Add hostile-input and fault-injection tests

Severity: Defense in depth  
Primary files: `test/StoreTest.cpp`, `test/SbcLegacyTest.cpp`, new fuzz targets, `CMakeLists.txt`.

The current tests validate crypto primitives, round trips, wrong passphrases, one legacy golden file, password-generator membership, and basic backup creation. They do not cover the resource and parser boundaries described above.

Recommended additions:

- Mutation tests for every modern header field and ciphertext boundary.
- Truncated input tests at every framing boundary.
- Exact tests for salt, tag, nonce, length, padding, and ciphertext block sizes.
- Legacy header, radix-64, CRC, framing, and decompression-size boundary tests.
- Backup fault injection: existing unwritable backup, failed removal, failed copy, failed commit, and full-disk simulation where practical.
- Fuzz targets for `decodeModern()`, `parseXml()`, `looksLikeLegacy()`/`legacyDecrypt()`, and strict header parsing. Fuzzers must use reduced/test KDF work or reject invalid parameters before invoking production-cost KDF operations.
- Sanitizer jobs on a supported Clang/GCC environment: AddressSanitizer and UndefinedBehaviorSanitizer. Add libFuzzer or another maintained fuzzing engine.
- Property tests for encode/decode round trips and tamper rejection.

Acceptance criteria:

- Every P0 parsing/resource fix has a regression test.
- Fuzzing a malformed input cannot trigger production-cost unbounded KDF work.
- CI runs the unit tests in at least one release-like configuration in addition to debug/sanitizer configurations.

Review verification performed:

- `ctest --test-dir cmake-build-debug --output-on-failure`
- Result: 34 of 34 tests passed in 44.65 seconds.
- The configured release tree referred to `sbc_test_NOT_BUILT` and `store_test_NOT_BUILT`, so release tests could not be run without building them.

### P3.2 Enable explicit compiler and linker hardening

Severity: Defense in depth  
Primary file: `CMakeLists.txt`.

Current observations:

- The repository does not explicitly request MSVC `/sdl`, Control Flow Guard, CET compatibility, or an elevated warning level.
- Some protections such as stack cookies, ASLR, and DEP may be compiler/linker defaults, but the build does not assert or verify them.

Recommended implementation:

1. Add target-scoped hardening options with compiler/version checks rather than global flags.
2. For MSVC, evaluate `/W4`, `/sdl`, `/guard:cf`, `/CETCOMPAT`, and Spectre-mitigated libraries where deployment constraints permit them.
3. For GCC/Clang builds, evaluate `-Wall -Wextra -Wpedantic`, stack protection, `_FORTIFY_SOURCE`, PIE, RELRO, and immediate binding as supported by the target platform.
4. Keep warnings manageable; fix warnings before enabling warnings-as-errors in CI.
5. Add a packaging check that verifies expected PE/ELF mitigation flags in release artifacts.
6. Do not assume static linking is inherently safer; static packages require rebuilding to receive dependency security fixes.

Acceptance criteria:

- CI records enabled hardening options for each supported toolchain.
- Release artifacts are checked for the intended platform mitigations.
- No hardening option silently breaks the static and DLL build variants.

### P3.3 Strengthen dependency pinning and update process

Severity: Defense in depth / supply-chain risk  
Primary file: `CMakeLists.txt`; supporting build documentation.

Current behavior:

- zlib and GoogleTest are fetched using named Git tags.
- Qt paths are hard-coded to a particular local installation series.

Recommended implementation:

1. Pin fetched Git dependencies to immutable commit hashes and record the expected upstream release/tag in comments.
2. Prefer release archives with cryptographic hashes when the build system supports a reliable workflow.
3. Add automated dependency inventory and periodic update checks.
4. Document the Qt and zlib security-update process for both shared and static distributions.
5. Generate an SBOM for packaged releases and retain exact compiler/dependency versions used to build them.

Acceptance criteria:

- Identical source configuration resolves immutable dependency revisions.
- Release documentation identifies every shipped third-party component and version.
- Static releases have a documented rebuild trigger when Qt, zlib, or the compiler runtime publishes a security update.

## Additional observations

### Password generator

`PasswordGenerator::alphabet()` intentionally duplicates digits in `UcLcDd` and `UcLcDds`, making output nonuniform over unique characters. At the default 16-character length the generated passwords are still expected to be strong, and `QRandomGenerator::system()` is the right source. Nevertheless, the distribution should be deliberate and documented. A simpler design would sample uniformly from unique characters and, if character-class requirements exist, guarantee one character from each required class followed by a secure shuffle.

Relevant file: `src/crypto/PasswordGenerator.cpp`, especially lines 15-38.

Suggested tests should check required character-class behavior, output length, allowed characters, and broad distribution sanity without fragile statistical thresholds.

### XML parsing

`QXmlStreamReader` is preferable to a DOM parser for bounded memory, but the application currently accumulates all `SiteEntry` objects and strings without explicit limits. Add entry and field-size ceilings as part of P0.2. Confirm and test that external entity resolution remains disabled; do not install an entity resolver for vault XML.

### Error classification

Wrong-passphrase and corrupt-file errors are intentionally similar in some cryptographic paths. Preserve resistance to decryption-oracle behavior, but distinguish unsupported versions and resource-limit violations when doing so does not depend on secret-derived state. Avoid exposing detailed post-decryption parsing errors to untrusted callers unless needed for diagnostics.

## Recommended implementation order

1. P0.1: fixed/bounded FPMQ1 KDF parameters.
2. P0.2: file, allocation, decompression, entry, and field limits.
3. P0.3: strict modern parser.
4. P3.1 subset: regression tests and fuzz harnesses for all P0 changes.
5. P1.2: explicit backup and rotation/migration policy.
6. P2.1: master-passphrase guidance and policy.
7. P2.2/P2.3: manual lock, shorter idle exposure, and clipboard-copy cleanup.
8. P1.1: reviewed FPMQ2 standard-AEAD design and migration implementation.
9. P3.2/P3.3: build hardening, immutable dependency pinning, SBOM, and release checks.

P1.1 should be designed early, but the P0 fixes should ship first because they are narrow, testable protections for all files the current application opens. Do not rush the new vault format merely to remove SBC; format and migration mistakes can permanently lock users out of their password databases.

## Files most likely to change

- `src/crypto/PasswordStore.cpp`
- `src/crypto/PasswordStore.h`
- `src/sbc/sbc_legacy.cpp`
- `src/sbc/sbc_legacy.h`
- `src/MainWindow.cpp`
- `src/MainWindow.h`
- `src/SiteDatabase.h`
- `src/ClipboardUtil.h`
- `src/PreferencesDialog.cpp`
- `src/PreferencesDialog.h`
- `src/ChangeMasterPassphraseDialog.cpp`
- `src/crypto/PasswordGenerator.cpp`
- `test/StoreTest.cpp`
- `test/SbcLegacyTest.cpp`
- `CMakeLists.txt`
- Format/security documentation under `doc/`

## Important compatibility constraints

- Never remove the ability to read checked-in legacy and FPMQ1 fixtures without an explicit product decision.
- Never lower the current FPMQ1 KDF cost as a performance optimization.
- Preserve verify-before-decrypt behavior.
- Preserve separate keys if FPMQ1 code is refactored.
- Preserve atomic save semantics.
- Test Unicode passphrases and entry data before changing string conversions or normalization.
- Treat backups and migrations as user-data operations: failure must not destroy the last readable vault.

