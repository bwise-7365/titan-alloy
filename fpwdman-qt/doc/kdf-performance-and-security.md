Copyright Ben Paul Wise. All Rights Reserved.

# Key derivation: why it was slow, and what the slowness is for

Opening or saving an encrypted database took about 3.3 seconds. The old FLTK app
was instant. This document records where that time went, what was done about it,
and -- more importantly -- which part of the cost must not be optimized away.

The short version: almost all of the time was waste, and it was removed. What
remains is deliberate, and removing it would be a security regression.

## The old app was fast because it was weak

FPwdMan v3.3.1 fed the typed passphrase almost directly into the cipher's key
schedule. There was no key derivation function at all. That is why it was
instant, and it is a weakness rather than a benchmark: an attacker holding a
stolen database file could test candidate passphrases as fast as they could run
the cipher, which is millions of guesses per second.

The modern FPMQ1 container deliberately makes each guess expensive. Some of the
time cost here is the security. The goal of this work was to remove the waste
around it, not to get back to FLTK's speed. Any future change that "fixes"
slowness by reducing the iteration count has given away the thing that was bought.

## Where the time actually went

The cost was entirely in PBKDF2. Nothing else was slow, and the test timings
proved it rather than suggested it:

    SbcLegacy.GoldenVectorQwerty      3 ms   whole legacy decrypt: SBC + zlib + XML
    Store.ModernRoundTrip          6614 ms   the same work + two key derivations

The SBC cipher, the XML, and zlib together accounted for about 3 ms. The other
99.9% was key derivation, at roughly 3.3 seconds per derivation -- one on open,
one on save. Three separate problems stacked up.

### 1. Three times the advertised iteration count

`kIterations` is 600,000, which reads like the OWASP floor for
PBKDF2-HMAC-SHA-256. The code was doing 1,800,000.

PBKDF2 produces output one hash-length block at a time, and it runs the **full**
iteration count **for every block**. The container asks for
`kSbcKeyLen + kMacKeyLen` = 64 + 32 = 96 bytes. SHA-256 emits 32. So:

    blocks = ceil(96 / 32) = 3
    3 blocks x 600,000 iterations = 1,800,000 HMAC-SHA-256 computations per open

Nobody chose that. It is what asking a 32-byte hash for 96 bytes costs, and it is
invisible at the call site, which is exactly why it survived. This remains true
today and is the largest lever still on the table -- see "What is left" below.

### 2. Every iteration re-did work that never changed

HMAC is defined as two hashes wrapped around fixed pads:

    HMAC(K, msg) = H( (K ^ opad) || H( (K ^ ipad) || msg ) )

    ipad = 0x36 repeated 64 times
    opad = 0x5C repeated 64 times

`K ^ ipad` and `K ^ opad` are each exactly one SHA-256 block. SHA-256 absorbs a
message block at a time, carrying a 256-bit running state, so feeding it
`K ^ ipad` costs one compression and leaves a state that depends **only on the
key**.

In PBKDF2 the key is the passphrase, and it never changes. That state is
therefore identical across all 600,000 iterations. Compute it once, copy it per
iteration, and each iteration costs **two** compressions. Recompute it every
time, and each iteration costs **four**. The old code recomputed it 1.8 million
times to arrive at the same 32 bytes.

### 3. An allocation per iteration

The inner loop called `QMessageAuthenticationCode::result()`, which returns a
`QByteArray` -- a heap allocation for every one of the 1.8 million iterations,
each holding 32 bytes, each immediately discarded.

### The root cause: Qt cannot express a fast PBKDF2

The fix for problem 2 is to snapshot a mid-hash state and clone it. Qt's
`QCryptographicHash` is not copyable and offers no way to export or restore its
internal state. It is a sealed box: start, feed, finish. There is no way to say
"resume from here" -- so a Qt-based PBKDF2 has no option but to re-absorb the
key every iteration and allocate every digest.

The old implementation was therefore about as good as Qt's crypto API permits.
The problem was the choice of API, not the code written against it.

## What was done

SHA-256, HMAC-SHA-256 and PBKDF2-HMAC-SHA-256 are now implemented in the
Qt-free `sbc` library (`src/sbc/sbc_kdf.{h,cpp}`), where the hash state is an
ordinary member that copies for free:

- `HmacSha256::setKey()` absorbs `K ^ ipad` and `K ^ opad` once and keeps both
  states. `begin()` copies one instead of re-hashing: two compressions per
  iteration rather than four.
- The derivation runs entirely in fixed-size stack buffers. No allocation.
- Key material in scratch buffers is wiped through a `volatile` pointer, so the
  compiler cannot delete stores it can prove nobody reads.

Measured effect, same machine, same tests:

    Store.ModernRoundTrip        6614 ms  ->  1435 ms
    Store.ModernWrongPassphrase  6713 ms  ->  1456 ms

That is 4.6x. One open or save went from about 3.3 s to about 0.7 s.

**The output did not change.** This is standard PBKDF2 either way, so existing
FPMQ1 files still decrypt bit-for-bit. `Store.Pbkdf2KnownAnswer` passes against
the identical expected digest it always did, and that is what guarantees it.

### On hand-writing crypto

Hand-writing cryptographic primitives is normally a bad idea, and the caution is
worth stating rather than waving away. Three things make it defensible here:

- The algorithms are fully specified and completely pinned by published test
  vectors. `test/SbcKdfTest.cpp` checks against FIPS 180-4 (SHA-256, including
  the million-character message), RFC 4231 (HMAC-SHA-256, including the
  over-long-key case), and RFC 7914 (PBKDF2-HMAC-SHA-256, multi-block). These
  are quoted from the standards, never captured from our own output -- a
  self-generated expected value would only prove the code agrees with itself.
- SHA-256 has no data-dependent branches and no lookup tables, so a
  straightforward implementation is naturally constant-time. There is no secret
  to leak through timing here, which is the usual way hand-rolled crypto fails.
- The alternative was linking OpenSSL, which would forfeit the single
  self-contained executable that the static build exists to produce.

This reasoning does **not** extend to inventing primitives or modes. It applies
only to implementing a well-specified standard and checking it against that
standard's own vectors.

## What is necessary for security

The parts below are load-bearing. Anyone tempted to make this faster should
understand what each one buys before touching it.

**The iteration count (`kIterations = 600000`).** This is the entire defense for
a stolen database file. It is meant to be slow: the cost is paid once by the
legitimate user who knows the passphrase, and paid per guess by an attacker who
does not. 600,000 is the OWASP floor for PBKDF2-HMAC-SHA-256. Lowering it to
make the app feel snappier directly multiplies an attacker's guessing rate. Do
not treat it as a performance knob. If open time must come down further, take it
out of the waste (below), not out of this.

**The per-file random salt (`kSaltLen = 16`, fresh on every save).** Without it,
the same passphrase yields the same key everywhere, and an attacker can
precompute one table against all databases at once. The salt makes the 600,000
iterations have to be paid separately for every file attacked. It is not secret
and is stored in the header in the clear; that is fine and expected.

**Encrypt-then-MAC, with the tag verified before decryption.** `decodeModern`
checks `hmacSha256(macKey, cipherBytes)` against the stored tag and throws
`WrongPassphrase` on a mismatch, before the ciphertext is fed to the cipher.
This is what stops a modified or corrupt file from being decrypted into
attacker-influenced plaintext. The ordering matters: verify first, decrypt
second.

**The constant-time tag comparison (`constantTimeEquals`).** A comparison that
returns early on the first differing byte leaks, through timing, how many bytes
matched -- which is enough to forge a tag one byte at a time. It must keep
comparing every byte regardless.

**Separate keys for the cipher and the MAC.** The 96 derived bytes are split
into a 64-byte cipher key and a distinct 32-byte MAC key. Using one key for both
roles is a standing invitation to interactions between the two primitives.

**The CSPRNG (`QRandomGenerator::system()`).** Salts and nonces come from the
OS entropy source, not a seeded PRNG.

**Zeroization.** Derived keys and plaintext are wiped after use. This is
best-effort by nature -- `QByteArray` is copy-on-write, so an earlier detach may
have left a copy that cannot be reached -- but best-effort is worth more than
nothing when a process is swapped or dumped.

## What is left

**The 3x from problem 1 is still there**, and it is the largest remaining win.
The fix is to stop stretching PBKDF2 across three output blocks: derive a single
32-byte block, then expand it to 96 bytes with HKDF, which costs two cheap HMAC
calls rather than 1.2 million extra iterations. That is exactly what HKDF is
for, and it is arguably better practice than asking PBKDF2 to produce a long
output. Expected result is roughly 0.2 s per open.

The catch is that it changes the derived bytes, so **every existing FPMQ1 file
would stop opening**. Doing it properly means a format version bump: write
FPMQ2, keep the FPMQ1 derivation on the read path, and migrate on next save. It
is deliberately not done here, because it is a format decision rather than a
performance one.

**Derivation still runs on the GUI thread.** At 0.7 s the window is unresponsive
for the duration, which reads as a freeze rather than as work. Moving it to a
worker with a busy indicator would fix the perception independently of the
number, and is worth doing regardless of whether the 3x is ever taken.

## The rule to remember

The 3 ms of cipher, XML and zlib is the app doing its job. The ~0.7 s of PBKDF2
is the app protecting a stolen file. Waste is anything that does neither -- and
that is what was removed here. When this comes up again, cut waste, not cost.

Copyright Ben Paul Wise. All Rights Reserved.
