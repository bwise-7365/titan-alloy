@echo off
rem Copyright Ben Paul Wise. All Rights Reserved.
rem Confirmation matches for the 2026-08-24 coarse survivors, strongest first.
rem 310 pairs (620 games) each at 500 ms; roughly 100 minutes per candidate at 8
rem threads, ~8.5 h for all five. Run from any directory (it cd's to the repo root);
rem output appends to confirm.log, PROMOTE/REJECT verdicts included.
rem Interruption-safe: rerunning this file skips confirmations already recorded in
rem confirm.csv and continues with the rest.
cd /d "%~dp0..\..\..\.."
set SWEEP=powershell -File tools\latrunculi-sweep.ps1 -Stage confirm -Bench ./cmake-build-release/latrunculi_game/latrunculi_bench.exe -Threads 8 -Ms 500 -OutDir doc/bench/sweeps/2026-08-24
set LOG=doc\bench\sweeps\2026-08-24\confirm.log

%SWEEP% -Candidate doc/bench/sweeps/2026-08-24/cand-vulnerableAxes.psd1 >> %LOG% 2>&1
if errorlevel 1 goto :fail
%SWEEP% -Candidate doc/bench/sweeps/2026-08-24/cand-mobility.psd1 >> %LOG% 2>&1
if errorlevel 1 goto :fail
%SWEEP% -Candidate doc/bench/sweeps/2026-08-24/cand-centre.psd1 >> %LOG% 2>&1
if errorlevel 1 goto :fail
%SWEEP% -Candidate doc/bench/sweeps/2026-08-24/cand-spearheadPairs.psd1 >> %LOG% 2>&1
if errorlevel 1 goto :fail
%SWEEP% -Candidate doc/bench/sweeps/2026-08-24/cand-strikers.psd1 >> %LOG% 2>&1
if errorlevel 1 goto :fail

echo all five confirmations finished - verdicts are in %LOG%
exit /b 0
:fail
echo a confirmation failed - see %LOG%
exit /b 1
rem Copyright Ben Paul Wise. All Rights Reserved.
