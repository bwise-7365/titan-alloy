# fpwdman-qt Security Review

**Date:** 2026-08-07  
**Reviewer:** Gemini CLI  
**Scope:** Core cryptography, memory zeroization, GUI secret lifetimes, and clipboard security.

---

## Priority 0: Denial of Service & Resource Exhaustion

### P0.1 PBKDF2 Iteration Count Validation
*   **File:** src/crypto/PasswordStore.cpp (decodeModern)
*   **Vulnerability:** iters is parsed from the unauthenticated FPMQ1 header and passed to PBKDF2 before MAC verification. An attacker can set iters=2147483647 to cause infinite CPU freeze.
*   **Recommendation:** Enforce iters == 600000 for FPMQ1 files. Throw CorruptFile immediately on mismatch.

### P0.2 Input File & Memory Allocation Boundaries
*   **File:** src/crypto/PasswordStore.cpp (openFile), src/sbc/sbc_legacy.cpp (parseEncFile, sbcUncompress)
*   **Vulnerability:** Unbounded reads and allocations. Legacy numPrintChar reserves memory before password validation. Decompression allocates rawLen from zlib header without limit.
*   **Recommendation:**
    *   Limit max file size to 32MB in openFile using QFileInfo::size().
    *   Validate numPrintChar against actual file size before reserving.
    *   Enforce max 64MB decompressed limit in sbcUncompress before allocation.

---

## Priority 1: Memory Safety & RAM Secret Leakage

### P1.1 SBCipher Key Schedule Leakage
*   *+Files:** src/sbc/sbc_core.h, src/sbc/sbc_core.cpp
*   **Vulnerability:** SBCipher lacks a destructor. Raw subkeys remain in key_ vector on the heap after deallocation.
*   **Recommendation:** Implement ~SBCipher() to zeroize key_ with a volatile pointer.

### P1.2 Copy-On-Write secureZero Detachment
*   **File:** src/crypto/PasswordStore.cpp (secureZero)
*   **Vulnerability:** secureZero calls b.data(). If QByteArray is shared, this detaches a copy, zeroing the copy and leaving the secret in the original buffer.
*   **Recommendation:** Verify unique ownership of sensitive buffers or use un-shared raw structures (e.g., std::vector<uint8_t>) for keys.

---

## Priority 2: Clipboard Security

### P2.1 Timer Lambda Capture of Plaintext
*   **File:** src/ClipboardUtil.h (copySensitive)
*   **Vulnerability:** QTimer::singleShot captures the password text by value. Plaintext remains in the Qt Event Loop for 30s.
*   **Recommendation:** Capture a SHA-256 hash of the password. Compare the hash of the current clipboard content to clear it.

### P2.2 Plaintext in Static lastCopied()
*   *+File:** src/ClipboardUtil.h (lastCopied)
*   **Vulnerability:** Static QString retains the last copied password in memory permanently until overwritten.
*   **Recommendation:** Store only the SHA-256 hash of the copied password instead of plaintext.

### P2.3 Windows Clipboard History Exposure
*   **File:** src/ClipboardUtil.h (copySensitive)
*   **Vulnerability:** Passwords appear in Windows Clipboard History (Win+V) and Cloud Sync.
*   **Recommendation:** Set Windows clipboard format metadata ExcludeClipboardContentFromMonitorProcessing to prevent history caching.

---

## Priority 3: Cryptography & Backups

### P3.1 Standard AEAD Migration (FPMQ2)
*   **Vulnerability:** SBC is a custom, non-standard cipher. CBC mode uses zero IV with partial block randomization.
*   **Recommendation:** Keep SBC read-only. Write new vaults using standard AES-256-GCM or ChaCha20-Poly1305 with Argon2id.

### P3.2 Stale Backups
*   **Vulnerability:** Database .bak copies remain encrypted under the old passphrase after rotation.
*   **Recommendation:** Re-encrypt the backup file using the new master passphrase.