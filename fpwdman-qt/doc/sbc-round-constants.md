Copyright Ben Paul Wise. All Rights Reserved.

# SBC version 1: a key schedule with a different subkey for every step

Status: implemented 2026-10-21 (`KeySchedule::V1` in `src/sbc/sbc_core.*`,
the FPMQ2 container in `src/crypto/PasswordStore.cpp`). Written the same day
from a cryptanalysis of `src/sbc/sbc_core.cpp`. It replaces an earlier version
of this note, whose formula was flawed (see "A tempting fix that does not
work").

One change is meant to serve both uses of SBC: SBC as a standalone cipher, and
SBC inside fpwdman. Both would call the same encipher and decipher with the
same key schedule. Only the source of the key differs.

## The weakness in version 0

SBC version 0 uses the same subkeys in every round. Step `i` uses
`subkey0(i) = qTransform(key[i mod 16])`, and the block position is `i mod 32`,
so the steps repeat exactly every 32. `perm1` uses no key and runs every 64
steps. The middle of the cipher is therefore one fixed map, 64 steps followed
by `perm1`, applied 11 times over. A test confirmed that the state after each
permutation is that same map applied to the state after the one before.

A cipher made of identical keyed rounds is open to a *slide attack*. If every
round is the same keyed function F, then E(F(P)) = F(E(P)). An attacker who
finds two plaintexts with P' = F(P) (a "slid pair") also has C' = F(C), and so
holds two input/output pairs for one round instead of 24. One round is far
weaker than the whole cipher. Finding a slid pair by chance takes about
2^(n/2) known plaintext blocks for an n-bit block, and that cost does not
depend on the number of rounds. Adding rounds cannot close this door.

How serious it is: a weakness on paper only. With 1024-bit blocks a slid pair
takes about 2^512 known blocks under one key, and fpwdman uses a fresh key on
every save. It is still a design defect, because it bypasses the round count,
which is the cipher's main safety margin.

## A tempting fix that does not work

The earlier note proposed adding a multiple of phi to the key word:

    subkey(i) = qTransform(key[i mod 16] + (i + 1) * phi)

Every step then has a different subkey, but the steps are related in a simple
way. Take a second key whose every word is larger by 64 * phi:
k' = k + 64 * phi. Step `i` under k' then uses exactly the subkey of step
`i + 64` under k. Sixty-four steps is a whole number of rounds and also matches
the `perm1` schedule, so the two keys give the same cipher shifted by 64 steps,
and the slide attack returns for related keys. Any constant that is a simple
(linear) function of the step number has this flaw. The constants must vary in
no regular way.

## The version-1 key schedule

At key setup, expand the key into one subkey for every step with HKDF-SHA-256
(RFC 5869, extract then expand):

    S[0 .. R*K - 1] = HKDF-SHA-256( IKM  = key,
                                    salt = empty,
                                    info = "SBC v1 subkeys" || INT32BE(K) || INT32BE(R),
                                    L    = 4 * R * K bytes )

Each run of 4 output bytes, packed big-endian, is one 32-bit subkey. Step `i`
uses `S[i]` in place of `subkey0(i)`. Nothing else in the cipher changes: fe4,
j0, perm1 and its schedule stay as they are. `decipherBlock` uses the same
table in reverse step order.

Why this design:

- Every step gets its own subkey, with no rule linking the subkeys of one key
  to those of another. That removes the slide attack, including the
  related-key form above.
- It accepts a key of any length and yields any number of subkeys, which suits
  SBC's sliding block size and round count.
- K and R are in `info` so that different block sizes or round counts give
  unrelated schedules. Without them, the 24-round schedule would be the start of
  the 25-round one, because HKDF output for a longer length begins with the
  output for a shorter one.
- The extract step is included because a standalone caller's key may not be
  uniformly random. fpwdman's key is, and extracting it again does no harm.
- The `sbc` library already has SHA-256 and HMAC (`sbc_kdf.cpp`); HKDF is a few
  lines on top. Cost for the default 32-word block and 24 rounds: 3,072 bytes,
  about 96 HMAC computations, done once per key.

Limit: HKDF can produce at most 255 * 32 = 8,160 bytes, that is R * K up to
2,040 words. The defaults use 768. The constructor refuses larger shapes with
`BadBlockSize`. If they are ever needed, run the expansion in chunks with a
chunk number appended to `info`; that would define a new version.

### Alternative: key word plus a public table of constants

If SBC should not lean on SHA-256 for its subkeys:

    subkey(i) = qTransform( key[i mod N] XOR RC[i] )
    RC[i]     = first 32 bits of SHA-256( "SBC-RC" || INT32BE(i) )

Because RC has no pattern, no change of key makes step `i` match step `i + d`,
so this also defeats the slide attack, and the key itself still enters every
step as in version 0. It is somewhat weaker than the HKDF schedule: each key
word is still used R * K / N times (48 by default), and the subkeys stay simply
related to the key. The HKDF schedule is the recommended one.

## Caution for standalone use

The key schedule is fast on purpose; it is not a key-derivation function. A
typed passphrase used directly as the key would be as cheap to guess as in the
old FLTK format. Standalone SBC must say plainly that a passphrase has to go
through PBKDF2 or Argon2 first, as fpwdman does.

## How fpwdman uses it: the FPMQ2 format

FPMQ2 gathers every change that alters the file's encryption, so the password
file is re-saved only once:

1. SBC version 1 (this note) for the cipher.
2. One PBKDF2 block, then HKDF, instead of three full PBKDF2 runs. Today 96
   bytes are requested, which runs PBKDF2 three times (1.8 million
   iterations), yet an attacker testing a guess needs only the 32-byte block
   holding the MAC key. The extra 1.2 million iterations add delay and no
   security. Instead:

        master = PBKDF2-HMAC-SHA-256(passphrase, salt, iterations, 32 bytes)
        sbcKey = HKDF-Expand(master, "FPMQ2 sbc", 64)
        macKey = HKDF-Expand(master, "FPMQ2 mac", 32)

   Opening then takes about a third of today's time. Alternatively, keep
   today's opening time and triple the iteration count, which triples an
   attacker's cost per guess.
3. A fixed iteration count, checked before any key derivation, as FPMQ1 now
   does (`decodeModern` rejects anything but 600000).
4. The HMAC covers the header as well as the ciphertext:
   `tag = HMAC(macKey, header lines || ciphertext)`, so a change to any
   parameter is detected.

The 16 random bytes at the front of the plaintext, the fresh 16-byte salt on
every save, and the separate cipher and MAC keys all stay as in FPMQ1.

Reading and writing: the app reads all three formats (the old radix-64 file,
FPMQ1 and FPMQ2) and always writes FPMQ2. Opening an older file and saving it
converts it.

## Checklist for the change

Items 1 to 5 were done on 2026-10-21; item 6 is the user's.

1. Done: `SbcCore.Version0KnownAnswer` pins version 0 with a vector captured
   before version 1 existed. `SbcCoreTest` had checked only round trips.
   `test/data/example-good-UTF-xml.sbc` (old format) and
   `example-good-UTF-xml-v2.sbc` (FPMQ1) are opened by `StoreTest`.
2. Done: HKDF in `sbc_kdf`, checked against RFC 5869 Appendix A cases 1-3.
3. Done: `SBCipher` takes a `KeySchedule` (V0 by default, for the old and FPMQ1
   read paths; V1 for FPMQ2) and computes its subkey table once at key setup.
   `SbcCore.Version1KnownAnswer` is checked against an independent Python
   reference, not against this code's own output.
4. Done: `PasswordStore` reads the old format, FPMQ1 and FPMQ2, and writes only
   FPMQ2.
5. Done: `StoreTest` covers the FPMQ2 round trip; FPMQ1 still decoding; both
   fixtures opening and migrating to FPMQ2 on save; a changed kdf name, stray
   header field, relabelled version, salt, tag or ciphertext being rejected;
   and wrong iteration counts being refused before derivation in both formats.
6. To do: re-save the password file, then delete its `.bak` and any older
   copies. An old-format copy under the same passphrase allows fast passphrase
   guessing, so change the master passphrase once nothing old remains.

Copyright Ben Paul Wise. All Rights Reserved.
