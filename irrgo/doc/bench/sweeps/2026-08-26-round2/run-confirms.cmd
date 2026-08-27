@echo off
rem Copyright Ben Paul Wise. All Rights Reserved.
rem Round-2 confirmation matches (310 pairs each at 500 ms) against the ROUND-2
rem incumbent, centre = 0.2 -- hence the -Incumbent flag on every call; without it
rem the candidates would be measured against the wrong opponent. Strongest first,
rem ~100 minutes each, ~8.5 h total. Interruption-safe: rerunning skips
rem confirmations already recorded in this round's confirm.csv.
cd /d "%~dp0..\..\..\.."
set SWEEP=powershell -File tools\latrunculi-sweep.ps1 -Stage confirm -Bench ./cmake-build-release/latrunculi_game/latrunculi_bench.exe -Threads 8 -Ms 500 -Incumbent doc/bench/sweeps/2026-08-24/cand-centre.psd1 -OutDir doc/bench/sweeps/2026-08-26-round2
set LOG=doc\bench\sweeps\2026-08-26-round2\confirm.log

%SWEEP% -Candidate doc/bench/sweeps/2026-08-26-round2/cand-vulnerableAxes.psd1 >> %LOG% 2>&1
if errorlevel 1 goto :fail
%SWEEP% -Candidate doc/bench/sweeps/2026-08-26-round2/cand-oneMoveCapturable.psd1 >> %LOG% 2>&1
if errorlevel 1 goto :fail
%SWEEP% -Candidate doc/bench/sweeps/2026-08-26-round2/cand-spearheadPairs.psd1 >> %LOG% 2>&1
if errorlevel 1 goto :fail
%SWEEP% -Candidate doc/bench/sweeps/2026-08-26-round2/cand-diagonalSupport.psd1 >> %LOG% 2>&1
if errorlevel 1 goto :fail
%SWEEP% -Candidate doc/bench/sweeps/2026-08-26-round2/cand-strikers.psd1 >> %LOG% 2>&1
if errorlevel 1 goto :fail

echo all five round-2 confirmations finished - verdicts are in %LOG%
exit /b 0
:fail
echo a confirmation failed - see %LOG%
exit /b 1
rem Copyright Ben Paul Wise. All Rights Reserved.
