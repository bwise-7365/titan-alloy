# Copyright Ben Paul Wise. All Rights Reserved.
#
# Weight-sweep driver for latrunculi_bench's A-vs-B mode. The bench measures one match;
# this script encodes the experiment policy around it -- stages, thresholds, promotion
# rules -- so the measurement tool stays policy-free. Protocol and the binomial sizing
# are documented in the approved plan (Stage C) and summarized per stage below.
#
# Windows PowerShell 5.1-safe: no pipeline-chain operators, no ternary.
#
# Stages (run in order; each reads the CSVs the earlier ones wrote into -OutDir):
#   baseline  incumbent vs incumbent, 100 pairs. Records the noise floor (quiet-game %,
#             mean captures) every later guardrail is measured against.
#   coarse    one-weight-at-a-time filter: each field at multipliers x1/4, x1/2, x2, x4
#             (or x0.7, x1.4 with -Narrow), 40 pairs each. Keeps a candidate iff its
#             win rate is >= 55% AND its quiet share is <= baseline + 5 points.
#   confirm   one candidate (-Candidate file.psd1) vs incumbent at 310 pairs (620
#             games). PROMOTE iff wins meet the one-sided 95% threshold AND quiet
#             share <= baseline + 3 AND mean captures >= 0.9 x baseline.
#   robust    candidate vs incumbent at 100 pairs on each of 6x6/12, 8x10/20, 10x12/30;
#             requires >= 50% on every size.
#
# Weight files (.psd1) hold a hashtable of overrides, e.g.:  @{ threat = 0.5 }
# Fields not named keep the engine defaults below. The defaults table MUST match
# EvalWeights in latrunculi_game/EvalWeights.h; the bench validates field names, so a
# renamed weight fails loudly here rather than sweeping a stale default.
#
# Examples (from the repo root, bash-style path for the exe):
#   powershell -File tools/latrunculi-sweep.ps1 -Stage baseline -Bench ./cmake-build-release/latrunculi_game/latrunculi_bench.exe -Threads 12
#   powershell -File tools/latrunculi-sweep.ps1 -Stage coarse   -Bench ... -Threads 12
#   powershell -File tools/latrunculi-sweep.ps1 -Stage confirm  -Bench ... -Candidate cand.psd1

param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('baseline', 'coarse', 'confirm', 'robust')]
    [string]$Stage,

    [Parameter(Mandatory = $true)]
    [string]$Bench,

    # .psd1 hashtable of incumbent weight overrides; empty = engine defaults.
    [string]$Incumbent = '',

    # .psd1 hashtable for the candidate (confirm and robust stages).
    [string]$Candidate = '',

    [int]$Threads = 8,
    [int]$Ms = 200,
    [uint64]$Seed = 777001,

    # Default: <repo>\doc\bench\sweeps\<date>\
    [string]$OutDir = '',

    # coarse: use the narrow multipliers x0.7 / x1.4 (later rounds).
    [switch]$Narrow
)

$ErrorActionPreference = 'Stop'
$inv = [System.Globalization.CultureInfo]::InvariantCulture

# ── EvalWeights defaults (keep in sync with latrunculi_game/EvalWeights.h) ──────
function Get-DefaultWeights {
    return [ordered]@{
        threat            = 0.25
        pair              = 0.02
        mobility          = 0.02
        centre            = 0.05
        vulnerableAxes    = -0.03
        oneMoveCapturable = -0.9
        spearheadPairs    = 0.15
        diagonalSupport   = 0.06
        deniedSquares     = 0.04
        strikers          = 0.05
        notchExposure     = -0.12
    }
}

function Read-WeightOverrides([string]$path) {
    if ($path -eq '') { return @{} }
    if (-not (Test-Path $path)) { throw "sweep: weight file not found: $path" }
    $table = Import-PowerShellDataFile -Path $path
    $valid = (Get-DefaultWeights).Keys
    foreach ($key in $table.Keys) {
        if ($valid -notcontains $key) {
            throw "sweep: unknown weight field '$key' in $path (valid: $($valid -join ', '))"
        }
    }
    return $table
}

function Get-EffectiveWeights($overrides) {
    $weights = Get-DefaultWeights
    foreach ($key in $overrides.Keys) { $weights[$key] = [double]$overrides[$key] }
    return $weights
}

# Full explicit vectors for both sides, so every CSV line is self-describing.
function Build-WeightArgs([string]$prefix, $weights) {
    $benchArgs = @()
    foreach ($key in $weights.Keys) {
        $benchArgs += ('{0}.{1}={2}' -f $prefix, $key, ([double]$weights[$key]).ToString('R', $inv))
    }
    return $benchArgs
}

# Runs one A-vs-B match and returns the CSV row it appended (as a PSCustomObject).
function Invoke-Match($candidateWeights, $incumbentWeights, [int]$pairs, [string]$csv,
                      [int]$rows, [int]$columns, [int]$perSide, [uint64]$matchSeed) {
    $benchArgs = @("pairs=$pairs", "ms=$Ms", "seed=$matchSeed", "threads=$Threads",
                   "rows=$rows", "columns=$columns", "perside=$perSide", "csv=$csv")
    $benchArgs += Build-WeightArgs 'wA' $candidateWeights
    $benchArgs += Build-WeightArgs 'wB' $incumbentWeights
    Write-Host ">> $Bench $($benchArgs -join ' ')"
    & $Bench @benchArgs | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "sweep: bench failed (exit $LASTEXITCODE)" }
    $rowsRead = @(Import-Csv -Path $csv)
    return $rowsRead[$rowsRead.Count - 1]
}

function Num($value) { return [double]::Parse($value, $inv) }

function Get-SignificanceThreshold([int]$games) {
    return [int][math]::Ceiling($games / 2.0 + 1.6449 * [math]::Sqrt($games / 4.0))
}

# Reads the noise floor the baseline stage recorded.
function Read-Baseline([string]$dir) {
    $path = Join-Path $dir 'baseline.csv'
    if (-not (Test-Path $path)) {
        throw "sweep: run -Stage baseline first ($path not found)"
    }
    $rowsRead = @(Import-Csv -Path $path)
    return $rowsRead[$rowsRead.Count - 1]
}

# ── Setup ───────────────────────────────────────────────────────────────────────
if (-not (Test-Path $Bench)) { throw "sweep: bench executable not found: $Bench" }
if ($OutDir -eq '') {
    $repoRoot = Split-Path -Parent $PSScriptRoot
    $OutDir = Join-Path $repoRoot ("doc\bench\sweeps\{0}" -f (Get-Date -Format 'yyyy-MM-dd'))
}
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Force $OutDir | Out-Null }
Write-Host "sweep stage '$Stage' -> $OutDir"

$incumbentWeights = Get-EffectiveWeights (Read-WeightOverrides $Incumbent)

# ── Stages ──────────────────────────────────────────────────────────────────────
switch ($Stage) {

    'baseline' {
        $csv = Join-Path $OutDir 'baseline.csv'
        $row = Invoke-Match $incumbentWeights $incumbentWeights 100 $csv 8 10 20 $Seed
        Write-Host ''
        Write-Host ("baseline noise floor: quiet {0}%  meanCaptures {1}  winsA {2}/200" -f `
            $row.quietPct, $row.meanCaptures, $row.winsA)
        Write-Host 'Later stages read these guardrails from baseline.csv.'
    }

    'coarse' {
        $baseline = Read-Baseline $OutDir
        $baseQuiet = Num $baseline.quietPct
        if ($Narrow) { $multipliers = @(0.7, 1.4) } else { $multipliers = @(0.25, 0.5, 2.0, 4.0) }
        $csv = Join-Path $OutDir 'coarse.csv'
        $survivors = @()
        foreach ($field in (Get-DefaultWeights).Keys) {
            foreach ($m in $multipliers) {
                if ([double]$incumbentWeights[$field] -eq 0.0) {
                    Write-Host "skip ${field} x${m}: incumbent value is 0, scaling is a no-op"
                    continue
                }
                $candidateWeights = Get-EffectiveWeights @{}
                foreach ($k in $incumbentWeights.Keys) { $candidateWeights[$k] = $incumbentWeights[$k] }
                $candidateWeights[$field] = [double]$incumbentWeights[$field] * $m
                $row = Invoke-Match $candidateWeights $incumbentWeights 40 $csv 8 10 20 $Seed
                $games = 2 * 40
                $winRate = (Num $row.winsA) / $games
                $quiet = Num $row.quietPct
                $kept = ($winRate -ge 0.55) -and ($quiet -le $baseQuiet + 5.0)
                if ($kept) { $verdict = 'KEEP' } else { $verdict = 'drop' }
                Write-Host ("{0}  {1} x{2} -> {3} = {4:P1} wins, quiet {5}%" -f `
                    $verdict, $field, $m, $candidateWeights[$field], $winRate, $quiet)
                if ($kept) {
                    $survivors += ('@{{ {0} = {1} }}' -f $field, ($candidateWeights[$field].ToString('R', $inv)))
                }
            }
        }
        Write-Host ''
        if ($survivors.Count -eq 0) {
            Write-Host 'No survivors: this round promotes nothing.'
        } else {
            Write-Host 'Survivors (save each as a .psd1 and run -Stage confirm):'
            foreach ($s in $survivors) { Write-Host "  $s" }
        }
    }

    'confirm' {
        if ($Candidate -eq '') { throw 'sweep: -Stage confirm needs -Candidate <file.psd1>' }
        $baseline = Read-Baseline $OutDir
        $baseQuiet = Num $baseline.quietPct
        $baseCaptures = Num $baseline.meanCaptures
        $candidateWeights = Get-EffectiveWeights (Read-WeightOverrides $Candidate)
        $csv = Join-Path $OutDir 'confirm.csv'
        $row = Invoke-Match $candidateWeights $incumbentWeights 310 $csv 8 10 20 $Seed
        $games = 2 * 310
        $threshold = Get-SignificanceThreshold $games
        $wins = [int](Num $row.winsA)
        $quiet = Num $row.quietPct
        $captures = Num $row.meanCaptures
        Write-Host ''
        Write-Host ("wins {0}/{1} (threshold {2}), quiet {3}% (limit {4}%), captures {5} (floor {6})" -f `
            $wins, $games, $threshold, $quiet, ($baseQuiet + 3.0), $captures, (0.9 * $baseCaptures))
        $strong = ($wins -ge $threshold)
        $lively = ($quiet -le $baseQuiet + 3.0) -and ($captures -ge 0.9 * $baseCaptures)
        if ($strong -and $lively) {
            Write-Host "PROMOTE: $Candidate becomes the incumbent for the next round."
        } elseif ($strong) {
            Write-Host 'REJECT: wins but dulls the game (quiet/capture guardrail failed).'
        } else {
            Write-Host 'REJECT: no significant strength gain.'
        }
    }

    'robust' {
        if ($Candidate -eq '') { throw 'sweep: -Stage robust needs -Candidate <file.psd1>' }
        $candidateWeights = Get-EffectiveWeights (Read-WeightOverrides $Candidate)
        $csv = Join-Path $OutDir 'robust.csv'
        $sizes = @(
            @{ rows = 6;  columns = 6;  perSide = 12 },
            @{ rows = 8;  columns = 10; perSide = 20 },
            @{ rows = 10; columns = 12; perSide = 30 }
        )
        $totalWins = 0
        $totalGames = 0
        $allAtLeastHalf = $true
        foreach ($size in $sizes) {
            $row = Invoke-Match $candidateWeights $incumbentWeights 100 $csv `
                $size.rows $size.columns $size.perSide $Seed
            $wins = [int](Num $row.winsA)
            $totalWins += $wins
            $totalGames += 200
            Write-Host ("{0}x{1}/{2}: candidate {3}/200" -f `
                $size.rows, $size.columns, $size.perSide, $wins)
            if ($wins -lt 100) { $allAtLeastHalf = $false }
        }
        $threshold = Get-SignificanceThreshold $totalGames
        Write-Host ''
        Write-Host ("pooled: {0}/{1} (threshold {2})" -f $totalWins, $totalGames, $threshold)
        if ($allAtLeastHalf -and ($totalWins -ge $threshold)) {
            Write-Host 'ROBUST: >= 50% on every size and pooled significance.'
        } else {
            Write-Host 'NOT ROBUST: size-dependent (suspect mobility/centre scaling).'
        }
    }
}
# Copyright Ben Paul Wise. All Rights Reserved.
